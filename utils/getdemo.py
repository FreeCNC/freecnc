#!/usr/bin/env python

import urllib2, os, sys, zipfile, md5
from cStringIO import StringIO

URLS = [
    ("ftp://ftp.westwood.com/pub/cc1/previews/demo/cc1demo1.zip",
    9367945, "7d770d38618e20796fbe642037f08de5"),
    ("ftp://ftp.westwood.com/pub/cc1/previews/demo/cc1demo2.zip",
    17797920, "bbe489d259c4e6d6cadb4a2544b764aa")]

def verify_download(data, digest):
    print "Verifying:",
    data.seek(0)
    downloaded_digest = md5.md5(data.read()).hexdigest()
    if digest != downloaded_digest:
        print "Failed!  Download left in failed_download.zip"
        # Current working directory is data/mix
        corrupted = file("../../utils/failed_download.zip", "w")
        data.seek(0)
        corrupted.write(data.read())
        corrupted.close()
        return False
    else:
        print "OK"
        return True

def extract_zip(data):
    print "Extracting:",
    zip = zipfile.ZipFile(data)
    for filename in zip.namelist():
        if filename.endswith(".MIX"):
            out = file(os.path.basename(filename).lower(), "w")
            out.write(zip.read(filename))
            out.close()
    print "done"

def download_url(url, size):
    bytes = 0
    f = urllib2.urlopen(url)
    data = StringIO()
    while True:
        # Don't want a newline added, so use this instead of print
        sys.stdout.write("Downloading %s (%.2fmb): %i%%\r" % (url, size/1024.0/1024.0, bytes*100.0 / size))
        # Ensure the output shows up now rather than later
        sys.stdout.flush()
        a = f.read(10240)
        bytes += 10240
        if not a:
            break
        data.write(a)
    print
    return data

def main():
    try:
        os.mkdir("../data/mix/td")
    except:
        pass
    os.chdir("../data/mix/td")

    print "Attempting to download Tiberian Dawn demo mix files..."
    for url, size, digest in URLS:
        data = download_url(url, size)
        if not verify_download(data, digest):
            return 1
        extract_zip(data)
    return 0

if __name__ == "__main__":
    sys.exit(main())
