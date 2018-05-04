import glob, re, os, subprocess, sys, tempfile

from optparse import OptionParser

def main():
    retval = 0
    usage = "Usage: %prog [options] "
    description = "Wrapper for drush export and ingest commands to migrate between repositories."

    version = "migrate v. 0.1"
    
    parser = OptionParser(usage=usage, description=description, version=version)
    parser.add_option("-p", "--pids", action="store", dest="pids", default="", help="Comma-separated list of PIDs to migrate.")
    parser.add_option("-a", "--source-alias", action="store", dest="source_alias", default="bdh", help="Drush site-alias of the source Islandora.")
    parser.add_option("-s", "--source-ssh-host", action="store", dest="source_ssh", help="Source SSH host.")
    parser.add_option("-u", "--source-ssh-user", action="store", dest="source_ssh_user", help="Source SSH username.")
    parser.add_option("-d", "--dest-alias", action="store", dest="dest_site_alias", help="Destination drush site alias.")
    parser.add_option("--verbose", action="store_true", dest="verbose", help="Verbose output")
    options, arguments = parser.parse_args()
    source_path = subprocess.getoutput("ssh " + options.source_ssh + " \"mktemp -d\"")
    if options.verbose:
        print("Using source temp directory " + source_path + " on " + options.source_ssh + ".")
    # Execute export drush command

    r, export_message = subprocess.getstatusoutput("drush " + options.source_alias + " --user=1 islandora-export-objects --pids=" + options.pids + " --pids-file-dir=" + source_path + " --include-datastreams=TRUE")
    if options.verbose:
        print(export_message)
        
    if r == 0:     
        with tempfile.TemporaryDirectory() as tempdirname:
            os.chdir(tempdirname)
            if options.verbose:
                print("Using local temp directory " + tempdirname + ".")
            tr, tr_message = subprocess.getstatusoutput("rsync -auv -e ssh " + options.source_ssh + ":" + source_path + "/* .")
            if options.verbose:
                print(tr_message)
            if tr == 0:
                foxml_files = ",".join(glob.glob("*.xml"))
                if options.verbose:
                    print("Importing foxml files: " + foxml_files)

                import_result, import_result_message = subprocess.getstatusoutput("drush " + options.dest_site_alias + " --user=1 islandora-import-objects --foxml-files-dir=" + tempdirname + " --foxml-files=" + foxml_files + " --use-exported-datastreams=TRUE")

                if options.verbose or import_result != 0:
                    print(import_result_message)
                if import_result != 0:
                    retval = 1
                    print("There was an error importing one or more PIDs. See above output for details.")
    # Delete temporary files on source server.
    if options.verbose:
        print("Cleaning up.")
    subprocess.getoutput("ssh " + options.source_ssh + " \"rm -rf " + source_path + "\"")
    return retval

if __name__ == '__main__':
    retval  = main()
    sys.exit(retval)

