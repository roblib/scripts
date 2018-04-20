import re, os, subprocess, sys
from optparse import OptionParser

def main():
    usage = "Usage: %prog [options] "
    description = "Ingest a list of Foxml files in a directory into a Fedora repository."

    version = "Ingest v. 0.1"
    parser = OptionParser(usage=usage, description=description, version=version)
    
    parser.add_option("-r", "--repo", action="store", dest="repo", default="localhost:8080", help="Repository server, including port, not including protcol or app path.")
    parser.add_option("-u", "--username", action="store", dest="fedora_user", default="fedoraAdmin", help="Fedora username")
    parser.add_option("-p", "--password", action="store", dest="fedora_pass", default="fedoraPassword", help="Fedora password")
    parser.add_option("-d", "--foxml-directory", action="store", dest="foxml_dir", default="export", help="Directory containing FOXML files to ingest.")
    parser.add_option("-m", "--format", action="store", dest="export_format", default="info:fedora/fedora-system:FOXML-1.1", help="Fedora export format")
    parser.add_option("--protocol", action="store", dest="protocol", default="http", help="Protocol (http, https)")

    # Command-line for export: 
    # $FEDORA_HOME/client/bin/fedora-export.sh bowingdownhome.ca:8080 fedoraAdmin bdhFedora@dmin bdh:2574 info:fedora/fedora-system:FOXML-1.1 migrate . http
    options, arguments = parser.parse_args()
    os.chdir(options.foxml_dir)
    files = [f for f in os.listdir('.') if os.path.isfile(f)]
    
    for foxml in files:
        subprocess.run([os.environ["FEDORA_HOME"] + "/client/bin/fedora-ingest.sh", 'f', foxml, options.export_format, options.repo, options.fedora_user, options.fedora_pass, options.protocol])
        
    print(options)
    

if __name__ == '__main__':
    main()
