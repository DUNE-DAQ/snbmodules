import os.path

def compare_raw_name(file1, file2):
    """Compare the name of two files from their path"""
    return os.path.basename(file1) == os.path.basename(file2)

def compare_raw_size(file1, file2):
    """Compare the size of two files from their path"""
    return os.path.getsize(file1) == os.path.getsize(file2)

def compare_raw_content(file1, file2, buffer_size=1024):
    """Compare the content of two files from their path"""
    buffer_count = 0
    with open(file1, 'rb') as f1, open(file2, 'rb') as f2:
        while True:
            data1 = f1.read(buffer_size)
            data2 = f2.read(buffer_size)
            
            if not data1 and not data2:
                return True
            if not data1 or not data2:
                return False
            if data1 != data2:
                print("Files are differents at bytes " + str(buffer_size * buffer_count) + " to " + str(buffer_size * (buffer_count + 1)))
                print(data1)
                print(data2)
                return False
            buffer_count += 1