from lxml import etree
from urllib.parse import urlparse
from functools import partial
import http.client

testfile = '/Users/aoneill/tmp/export/bdh_479.xml'

xmlns = {"foxml": "info:fedora/fedora-system:def/foxml#",
             "xsi": "http://www.w3.org/2001/XMLSchema-instance",
             }

class Foxml:
    def __init__(self, foxml_filename):
        self.tree = self.open_foxml_file(foxml_filename)

    def open_foxml_file(self, foxml_filename):
        with open(foxml_filename, 'r') as foxmlfile:
            data = foxmlfile.read()
        tree = etree.XML(data)
        return tree

    def pid(self):
        pids = self.tree.xpath('/foxml:digitalObject/@PID', namespaces=xmlns)
        return pids[0]

    def save(self, filename = ''):
        str = etree.tostring(self.tree)
        with open(filename, 'wb') as f:
            f.write(str)
        return str

    
class FoxmlCleaner:

    def __init__(self, foxml_filename):
        self.foxml = Foxml(foxml_filename)
        self.bad_datastream_version_is_latest = []
        self.no_good_datastream_verssions = []
        self.bad_datastream_versions = []
        
    def get_status_code(self, host, port = 80, path = "/"):
        """ This function retreives the status code of a website by requesting
        HEAD data from the host. This means that it only requests the headers.
        If the host cannot be reached or something else goes wrong, it returns
        None instead.
        """
        try:
            conn = http.client.HTTPConnection(host, port=port)
            conn.request("GET", path)
            return conn.getresponse().status
        except http.client.HTTPException:
            return None



    
    def get_bad_datastream_versions(self):
        self.bad_datastream_versions = []
        hrefs = self.get_datastream_version_locations(self.foxml.tree)
        bad_datastreams = []
        for href in hrefs:
            ds_url = href.get("REF")
            ds_url_parsed = urlparse(ds_url)
            code = self.get_status_code(ds_url_parsed.hostname, ds_url_parsed.port, ds_url_parsed.path)

            if str(code) == '500':
                self.bad_datastream_versions.append(href)

        return self.bad_datastream_versions

    
    def get_datastream_version_locations(self, tree):
        hrefs = tree.xpath("//foxml:contentLocation", namespaces=xmlns)
        return hrefs        

    def remove_bad_datastream_version(self, bad_element):
        bad_element.getparent().getparent().remove(bad_element.getparent())

#    def datastream_version_is_most_recent(self, ds):
    def datastream_version_number(self, ds):
        return dict(ds.getparent().items())['ID'].split('.')[1]

    def datastream_version_siblings_versions(self, ds):
        return [dict(i.items())['ID'].split('.')[1] for i in ds.getparent().getparent().getchildren()]

    def datastream_version_is_newest_version(self, ds):
        versions = [int(x) for x in self.datastream_version_siblings_versions(ds)]
        this_version = int(self.datastream_version_number(ds))
        return this_version == max(versions)
    
    def get_datastream_version_dsid(self, ds):
        return dict(ds.getparent().getparent().items())['ID']
