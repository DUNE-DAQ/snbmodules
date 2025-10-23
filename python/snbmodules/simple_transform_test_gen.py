import fddetdataformats
import detdataformats
import conffwk
import os
from collections import namedtuple
from daqconf.assets import resolve_asset_file
import sys

GeoID = namedtuple('GeoID', ['det_id', 'crate_id', 'slot_id', 'stream_id'], defaults=[0,0,0,0])

def get_file_info(filename):

    if "asset:" in filename:
        filename = resolve_asset_file(filename)

    with open(filename, 'rb') as ff:

        # Get file type from first DaqEthHeader
        hdr_size = detdataformats.DAQEthHeader.sizeof()
        hdr_bin = ff.read(hdr_size)
        hdr = detdataformats.DAQEthHeader(hdr_bin)
        detector_id = hdr.det_id
        ff.seek(0)

        frame_size = 0
        frame_type = "wibeth"
        if detector_id in [2, 8, 9]: # PDS
            frame_size = fddetdataformats.DAPHNEFrame.sizeof()
            frame_type = "daphne"
        elif detector_id  == 11: # TDE
            frame_size = fddetdataformats.TDEEthFrame.sizeof()
            frame_type = "tdeeth"
        else:
            frame_size = fddetdataformats.WIBEthFrame.sizeof()

        frame_counter = 0
        first_timestamp = -1
        last_timestamp = -1
        geo_id = GeoID()
        while True: # Loop over frames in file
            frame_bin = ff.read(frame_size)
            if not frame_bin:
                break
            frame_counter += 1

            if frame_type == 'wibeth':
                frame = fddetdataformats.WIBEthFrame(frame_bin)
            if frame_type == 'tdeeth':
                frame = fddetdataformats.TDEEthFrame(frame_bin)
            if frame_type == 'tde16':
                frame = fddetdataformats.TDE16Frame(frame_bin)
            if frame_type == 'daphne':
                frame = fddetdataformats.DAPHNEFrame(frame_bin)
            if frame_type == 'daphne-stream':
                frame = fddetdataformats.DAPHNEStreamFrame(frame_bin)


            timestamp = frame.get_timestamp()
            if first_timestamp == -1 or timestamp < first_timestamp:
                first_timestamp = timestamp
            if last_timestamp == -1 or timestamp > last_timestamp:
                last_timestamp = timestamp
            
            dheader = frame.get_daqheader()
            stream_id = 0
            if isinstance(dheader, detdataformats._daq_detdataformats_py.DAQHeader):
                stream_id = dheader.link_id
            else:
                stream_id = dheader.stream_id
            geo_id = GeoID(det_id=dheader.det_id, crate_id=dheader.crate_id, slot_id=dheader.slot_id, stream_id=stream_id)

    return {"frame_count": frame_counter, "first": first_timestamp, "last": last_timestamp, "geo_id": geo_id}

def generate_transform_objs(oksfile, files, trigger_mode):

    schemafiles = [
        "schema/confmodel/dunedaq.schema.xml",
        "schema/appmodel/application.schema.xml",
        "schema/appmodel/fdmodules.schema.xml",
        "schema/appmodel/trigger.schema.xml",
    ]
    dal = conffwk.dal.module("generated", schemafiles)
    db = conffwk.Configuration("oksconflibs")
    db.create_db(oksfile, schemafiles)

    file_infos = {}
    files_by_geoid = {}
    for file in files:
        print(f"Reading file info for {file}")
        this_info = get_file_info(file)
        if this_info["geo_id"] in files_by_geoid.keys():
            files_by_geoid[this_info["geo_id"]].append(file)
        else:
            files_by_geoid[this_info["geo_id"]] = [file]
        file_infos[file] = this_info

    groups = []
    streams = []
    senders = []
    source_id = 0
    print(f"New nic adding nic with id nic-{0}")
    nic_dal = dal.NetworkInterface(f"snb-nic-{0}")
    db.update_dal(nic_dal)

    for geoid,geoid_files in files_by_geoid.items():
        print (f"Generating {geoid=}")
        first_file = geoid_files[0]
        first_file_info = file_infos[first_file]

        geo_dal = dal.GeoId(
            f"geioId-{source_id}",
            detector_id=geoid.det_id,
            crate_id=geoid.crate_id,
            slot_id=geoid.slot_id,
            stream_id=geoid.stream_id,
        )
        db.update_dal(geo_dal)
        stream = dal.DetectorStream(
            f"stream-{source_id}",
            source_id=source_id,
            geo_id=geo_dal,
        )
        db.update_dal(stream)
        streams.append(stream)
        db.commit()

        sender_dal = dal.FakeDataSender(
            f"sender-{source_id}",
            streams=[stream],
            uses=nic_dal
        )
        db.update_dal(sender_dal)
        senders.append(sender_dal)
        db.commit()

        rec_dal = dal.FileReaderReceiver(f"dataRec-{source_id}", uses=nic_dal)
        db.update_dal(rec_dal)
        detconn_dal = dal.NetworkDetectorToDaqConnection(
            f"det-conn-{source_id}",
            net_receiver=rec_dal,
            net_senders=senders
        )
        db.update_dal(detconn_dal)
        groups.append(detconn_dal)

        file_source_dal = dal.SNBFileSourceParameters(f"snb-files-det-conn-{source_id}", data_files=geoid_files, input_buffer_size=5777280, 
                        file_compression_algorithm="None",)
        db.update_dal(file_source_dal)

        senders = []
        source_id = source_id + 1


    triggers=[]

    if trigger_mode == "per-file": # Make a trigger matching the contents of each file

        for file,file_info in file_infos.items():
            first = file_info["first"]
            last = file_info["last"]
            pctmt_dal = dal.PreconfiguredTriggerModuleTrigger(f"pc-trig-{first}", timestamp_start=first, timestamp_end=last)
            match = False
            for trig in triggers:
                if trig.timestamp_start == first:
                    match = True
                    break;

            if not match:
                db.update_dal(pctmt_dal)
                triggers.append(pctmt_dal)
    elif trigger_mode == "aligned-chunks": # Make a "start", "body" and "end" trigger
        earliest_start = -1
        latest_start = -1
        earliest_end = -1
        latest_end = -1

        for file,file_info in file_infos.items():
            first = file_info["first"]
            last = file_info["last"]
            if earliest_start == -1 or first < earliest_start:
                earliest_start = first
            if latest_start == -1 or first > latest_start:
                latest_start = first
            if earliest_end == -1 or last < earliest_end:
                earliest_end = last
            if latest_end == -1 or last > latest_end:
                latest_end = last

        if earliest_end < latest_start:
            print(f"Warning: Latest window start time is after earliest end time! Only creating \"start\" and \"end\" triggers!")
            earliest_end = latest_start

        start_pctmt_dal = dal.PreconfiguredTriggerModuleTrigger(f"pc-trig-{earliest_start}", timestamp_start=earliest_start, timestamp_end=latest_start)
        db.update_dal(start_pctmt_dal)
        triggers.append(start_pctmt_dal)
        if earliest_end > latest_start:
            body_pctmt_dal = dal.PreconfiguredTriggerModuleTrigger(f"pc-trig-{latest_start}", timestamp_start=latest_start, timestamp_end=earliest_end)
            db.update_dal(body_pctmt_dal)
            triggers.append(body_pctmt_dal)
        end_pctmt_dal = dal.PreconfiguredTriggerModuleTrigger(f"pc-trig-{earliest_end}", timestamp_start=earliest_end, timestamp_end=latest_end)
        db.update_dal(end_pctmt_dal)
        triggers.append(end_pctmt_dal)
    else: # One big trigger
        earliest_start = -1
        latest_end = -1

        for file,file_info in file_infos.items():
            first = file_info["first"]
            last = file_info["last"]
            if earliest_start == -1 or first < earliest_start:
                earliest_start = first
            if latest_end == -1 or last > latest_end:
                latest_end = last
        pctmt_dal = dal.PreconfiguredTriggerModuleTrigger(f"pc-trig-{earliest_start}", timestamp_start=earliest_start, timestamp_end=latest_end)
        db.update_dal(pctmt_dal)
        triggers.append(pctmt_dal)

    tc_readout_dal = dal.TCReadoutMap(f'tc-readout-snb', tc_type_name="kSupernova", time_before=0, time_after=1000)
    db.update_dal(tc_readout_dal)
    pct_dal = dal.PreconfiguredTriggerModuleConf(f'pc-trig-conf', template_for="PreconfiguredTriggerModule", wait_time_ms=1000, triggers=triggers, tc_readout=tc_readout_dal)
    db.update_dal(pct_dal)

    db.commit()
