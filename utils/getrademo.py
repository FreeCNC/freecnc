from getdemo import *

URLS = [
    ("ftp://ftp.westwood.com/pub/redalert/previews/demo/ra95demo.zip",
    27076646, "b44ab9ec1bc634ea755587d1988e3722")]

def main():
    try:
        os.mkdir("../data/mix/ra")
    except:
        pass
    os.chdir("../data/mix/ra")

    print "Attempting to download Red Alert demo mix files..."
    for url, size, digest in URLS:
        data = download_url(url, size)
        if not verify_download(data, digest):
            return 1
        extract_zip(data)
    return 0

if __name__ == "__main__":
    sys.exit(main())