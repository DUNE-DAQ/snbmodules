// This is the configuration schema for snbmodules
local moo = import "moo.jsonnet";

local sdc = import "fddaqconf/confgen.jsonnet";
local fddaqconfgen = moo.oschema.hier(sdc).dunedaq.fddaqconf.confgen;

local stypes = import "daqconf/types.jsonnet";
local types = moo.oschema.hier(stypes).dunedaq.daqconf.types;

local sctb = import "ctbmodules/ctbmodule.jsonnet";
local ctbmodule = moo.oschema.hier(sctb).dunedaq.ctbmodules.ctbmodule;

local sboot = import "daqconf/bootgen.jsonnet";
local bootgen = moo.oschema.hier(sboot).dunedaq.daqconf.bootgen;

local sdetector = import "daqconf/detectorgen.jsonnet";
local detectorgen = moo.oschema.hier(sdetector).dunedaq.daqconf.detectorgen;

local sdaqcommon = import "daqconf/daqcommongen.jsonnet";
local daqcommongen = moo.oschema.hier(sdaqcommon).dunedaq.daqconf.daqcommongen;

local stiming = import "daqconf/timinggen.jsonnet";
local timinggen = moo.oschema.hier(stiming).dunedaq.daqconf.timinggen;

local shsi = import "daqconf/hsigen.jsonnet";
local hsigen = moo.oschema.hier(shsi).dunedaq.daqconf.hsigen;

local sreadout = import "fddaqconf/readoutgen.jsonnet";
local readoutgen = moo.oschema.hier(sreadout).dunedaq.fddaqconf.readoutgen;

local strigger = import "daqconf/triggergen.jsonnet";
local triggergen = moo.oschema.hier(strigger).dunedaq.daqconf.triggergen;

local sdataflow = import "daqconf/dataflowgen.jsonnet";
local dataflowgen = moo.oschema.hier(sdataflow).dunedaq.daqconf.dataflowgen;

local sdqm = import "daqconf/dqmgen.jsonnet";
local dqmgen = moo.oschema.hier(sdqm).dunedaq.daqconf.dqmgen;

local ssnbmodules = import "snbmodules/snbmodulesgen.jsonnet";
local snbmodulesgen = moo.oschema.hier(ssnbmodules).dunedaq.snbmodules.snbmodulesgen;

local ns = "dunedaq.snbmodules.confgen";
local s = moo.oschema.schema("dunedaq.snbmodules.confgen");

local cs = {

  snbmodules_gen: s.record('snbmodules_gen', [
    s.field('detector',    detectorgen.detector,   default=detectorgen.detector,     doc='Boot parameters'),
    s.field('daq_common',  daqcommongen.daq_common, default=daqcommongen.daq_common,   doc='DAQ common parameters'),
    s.field("boot", bootgen.boot, default=bootgen.boot, doc="Boot parameters"),
    s.field('snbmodules',     snbmodulesgen.snbmodules,    default=snbmodulesgen.snbmodules,      doc='SNBModules parameters'),
  ]),

  snbmodules_multiru_multisnb_gen: s.record('snbmodules_multiru_multisnb_gen', [
    s.field('detector',    detectorgen.detector,   default=detectorgen.detector,     doc='Boot parameters'),
    s.field('daq_common',  daqcommongen.daq_common, default=daqcommongen.daq_common,   doc='DAQ common parameters'),
    s.field('boot',        bootgen.boot,    default=bootgen.boot,      doc='Boot parameters'),
    s.field('dataflow',    dataflowgen.dataflow,   default=dataflowgen.dataflow,     doc='Dataflow paramaters'),
    s.field('dqm',         dqmgen.dqm,        default=dqmgen.dqm,          doc='DQM parameters'),
    s.field('hsi',         hsigen.hsi,        default=hsigen.hsi,          doc='HSI parameters'),
    // s.field('ctb_hsi',     fddaqconfgen.ctb_hsi,    default=fddaqconfgen.ctb_hsi,      doc='CTB parameters'),
    s.field('readout',     readoutgen.readout,    default=readoutgen.readout,      doc='Readout parameters'),
    s.field('timing',      timinggen.timing,     default=timinggen.timing,       doc='Timing parameters'),
    s.field('trigger',     triggergen.trigger,    default=triggergen.trigger,      doc='Trigger parameters'),
    s.field('snbmodules',     snbmodulesgen.snbmodules,    default=snbmodulesgen.snbmodules,      doc='SNBModules parameters'),
  ]),

};

// Output a topologically sorted array.
stypes + sctb + sdc + sboot + sdetector + sdaqcommon + stiming + shsi + sreadout + strigger + sdataflow + sdqm + ssnbmodules + moo.oschema.sort_select(cs, ns)
