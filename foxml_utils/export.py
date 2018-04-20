import re, os, subprocess, sys
from optparse import OptionParser

def main():
    usage = "Usage: %prog [options] "
    description = "Parse a list of PIDs, export them from the specified repo and "\
    + "performs the options actions on the outputted FOXML files."
    version = "Export v. 0.1"
    parser = OptionParser(usage=usage, description=description, version=version)
    
    parser.add_option("-r", "--repo", action="store", dest="repo", default="localhost:8080", help="Repository server, including port, not including protcol or app path.")
    parser.add_option("-u", "--username", action="store", dest="fedora_user", default="fedoraAdmin", help="Fedora username")
    parser.add_option("-p", "--password", action="store", dest="fedora_pass", default="fedoraPassword", help="Fedora password")
    parser.add_option("-f", "--pids-file", action="store", dest="pids_file", default="", help="File containing a list of PIDs to import.")
    parser.add_option("--pids", action="store", dest="pids", default="", help="Comma-separated list of PIDs to export.")
    parser.add_option("-m", "--format", action="store", dest="export_format", default="info:fedora/fedora-system:FOXML-1.1", help="Fedora export format")
    parser.add_option("-t", "--export-type", action="store", dest="export_type", default="default", help="Export type (public, migrate, archive, default)")
    parser.add_option("-d", "--destination", action="store", dest="destination_directory", default=".", help="Destination directory. (default is current directory)")
    parser.add_option("--protocol", action="store", dest="protocol", default="http", help="Protocol (http, https)")

    # Command-line for export: 
    # $FEDORA_HOME/client/bin/fedora-export.sh bowingdownhome.ca:8080 fedoraAdmin bdhFedora@dmin bdh:2574 info:fedora/fedora-system:FOXML-1.1 migrate . http
    options, arguments = parser.parse_args()
    if options.pids == "":
        pids = open(options.pids_file)
    else:
        pids = options.pids.split(',')

    for pid in pids:
        subprocess.run([os.environ["FEDORA_HOME"] + "/client/bin/fedora-export.sh", options.repo, options.fedora_user, options.fedora_pass, pid, options.export_format, options.export_type, options.destination_directory, options.protocol])
        
    print(options)
    

if __name__ == '__main__':
    main()
