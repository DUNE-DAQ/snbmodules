import os.path

def check_contain_no_errors(bookkeeper_file_path):
    """Check for errors in the bookkeeper file"""
    with open(bookkeeper_file_path, 'r') as bookkeeper_file:
        for line in bookkeeper_file:
            if line.find("ERROR") != -1:
                return False
    return True

def check_transfer_finished(bookkeeper_file_path, file_name):
    """Check that the transfer is finished in the bookkeeper file"""
    file_found = False
    with open(bookkeeper_file_path, 'r') as bookkeeper_file:
        for line in bookkeeper_file:
            if line.find(file_name) != -1:
                file_found = True
                if line.find("FINISHED") == -1:
                    return False
    return file_found

def check_transfer_uploading(bookkeeper_file_path, file_name):
    """Check that the transfer is uploading in the bookkeeper file"""
    print('Check that the transfer is uploading in the bookkeeper file:', file_name)
    file_found = False
    with open(bookkeeper_file_path, 'r') as bookkeeper_file:
        for line in bookkeeper_file:
            if line.find(file_name) != -1:
                file_found = True
                if line.find("UPLOADING") == -1:
                    return False
    return file_found

def check_transfer_downloading(bookkeeper_file_path, file_name):
    """Check that the transfer is downloading in the bookkeeper file"""
    print('Check that the transfer is downloading in the bookkeeper file:', file_name)
    file_found = False
    with open(bookkeeper_file_path, 'r') as bookkeeper_file:
        for line in bookkeeper_file:
            if line.find(file_name) != -1:
                file_found = True
                if line.find("DOWNLOADING") == -1:
                    return False
    return file_found

def check_transfer_state(bookkeeper_file_path, file_name):
    """Check transfer state in the bookkeeper file. Returning list with count(uploading, downloading, finished)"""
    print('Check the transfer state in the bookkeeper logs for file:', file_name)
    file_found = False
    state = [0, 0, 0]
    with open(bookkeeper_file_path, 'r') as bookkeeper_file:
        for line in bookkeeper_file:
            if line.find(file_name) != -1:
                file_found = True
                if "UPLOADING" in line:
                    state[0] += 1
                elif "DOWNLOADING" in line:
                    state[1] += 1
                elif "FINISHED" in line:
                    state[2] += 1
    return state

