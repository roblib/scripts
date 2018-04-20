import re, os, subprocess, sys
from optparse import OptionParser

from foxml import Foxml, FoxmlCleaner
import dict_to_csv

report = []

def main():
    usage = "Usage: %prog [options] "
    description = "Parse FOXML files and remove any datastream version entries whose "\
                  + "hrefs lead to missing or corrupted resources."
    version = "FOXML Invalid Datastream Cleaner v. 0.1."
    parser = OptionParser(usage=usage, description=description, version=version)

    parser.add_option("-f", "--foxml-files", action="store", dest="foxml_files", default="", help="Comma-separated list of FOXML files to process.")
    parser.add_option("-d", "--directory", action="store", dest="foxml_directory", default="", help="Directory to process all files i.")
    parser.add_option("-o", "--output-directory", action="store", dest="output_directory", default="output", help="Directory where output files will be placed.")

    options, arguments = parser.parse_args()

    options.arguments = parser.parse_args()

    if options.foxml_directory == "" and options.foxml_files == '':
        print("Please specify either a comma-separated list of FOXML files wih -f or a directory with -d.")
        exit(1)
    
    if options.foxml_directory != "":
        files = [f for f in os.listdir('.') if os.path.isfile(f) and f.endswith('.xml')]
    else:
        files = options.foxml_files.split(',')

    for foxml_file in files:
        print("Loading: " + foxml_file)
        foxml_tree = FoxmlCleaner(foxml_file)
        bad_datastream_versions = foxml_tree.get_bad_datastream_versions()
        
        pid = foxml_tree.foxml.pid()
        print("Processing " + pid)
        for bad_datastream_version in bad_datastream_versions:
            newest = False
            dsid = foxml_tree.get_datastream_version_dsid(bad_datastream_version)
            if foxml_tree.datastream_version_is_newest_version(bad_datastream_version):
                newest = True
            grandparent = bad_datastream_version.getparent().getparent()
            foxml_tree.remove_bad_datastream_version(bad_datastream_version)
            missing_datastream = grandparent.getchildren() == []
            report.append({'PID': pid, 'DSID': dsid, 'Newest': newest, 'Missing Datastream': missing_datastream})
            
        if bad_datastream_versions != []:
            if not os.path.exists('clean'):
                os.makedirs('clean')
            foxml_tree.foxml.save('clean/' + foxml_file + ".clean.xml")
            
    dict_to_csv.convert_dict_to_csv(report, 'report.csv')
    print("Exported cleaned FOXML files to ./clean")
    print("Wrote report.csv")
    
        


    
if __name__ == '__main__':
    main()
