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