import glob, re, os, subprocess, sys, tempfile

from optparse import OptionParser

def main():
    usage = "Usage: %prog [options] "
    description = "Wrapper for drush export and ingest commands to migrate between repositories."

    version = "migrate v. 0.1"
    
    parser = OptionParser(usage=usage, description=description, version=version)
    parser.add_option("-p", "--pids", action="store", dest="pids", default="", help="Comma-separated list of PIDs to migrate.")
    parser.add_option("-a", "--source-alias", action="store", dest="source_alias", default="bdh", help="Drush site-alias of the source Islandora.")
    parser.add_option("-s", "--source-ssh-host", action="store", dest="source_ssh", help="Source SSH host.")
    parser.add_option("-u", "--source-ssh-user", action="store", dest="source_ssh_user", help="Source SSH username.")
    parser.add_option("-d", "--dest-site-alias", action="store", dest="dest_site_alias", help="Destination drush site alias.")

    parser.add_option("-t", "--source-tmp-folder", action="store", dest="source_tmp", help="Temporary folder on the source host.")
    options, arguments = parser.parse_args()
    source_path = subprocess.getoutput("ssh " + options.source_ssh + " \"mktemp -d\"")
    # Execute export drush command
    # drush --user=1 islandora-export-objects --pids-file-dir=/Users/aoneill/tmp/expor
    #t --pids-filename=test.txt  --include-datastreams=TRUE

    r, export_message = subprocess.getstatusoutput("drush " + options.source_alias + " --user=1 islandora-export-objects --pids=" + options.pids + " --pids-file-dir=" + source_path + " --include-datastreams=TRUE")

    if r == 0:     
        with tempfile.TemporaryDirectory() as tempdirname:
            os.chdir(tempdirname)
            tr, tr_message = subprocess.getstatusoutput("rsync -auv -e ssh " + options.source_ssh + ":" + source_path + "/* .")
            if tr == 0:
                foxml_files = ",".join(glob.glob("*.xml"))
                # drush --user=1 islandora-import-objects --foxml-files-dir=/home/aoneill/export - -foxml-files-list=/home/aoneill/export/bdh_remaining_files.txt --use-exported-da tastreams=TRUE
                subprocess.getoutput("drush " + options.dest_site_alias + " --user=1 islandora-import-objects --foxml-files-dir=" + tempdirname + " --foxml-files=" + foxml_files + " --use-exported-datastreams=TRUE")
    # Delete temporary files on source server.
    subprocess.getoutput("ssh " + options.source_ssh + " \"rm -rf " + source_path + "\"")

if __name__ == '__main__':
    main()

