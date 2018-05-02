scripts
=======
Repo for various robertson library scripts.  

Currently the drush scripts are being migrated to use tuque.  Any scripts that have been updated have been moved to the drupal7 folder.  Ideally only one version the scripts should be installed on a server as there maybe conflicts if both versions exist on the same server.

Setup
-----
To make the Drush commands accessible to Drush, make a shortcut to them in the .drush directory in your home directory, e.g., 

```bash
# If this repository is located in ~/dev/scripts.
mkdir ~/.drush
ln -s ~/scripts/drush/drupal7 ~/.drush/islandora
# Force drush to reload its list of commands
drush cc drush
```

Data Migration Drush Commands
-----------------------------

In order to migrate content between repositories that is protected by XACML POLICY streams you can now use the 'islandora-export-objects' and 'islandora-import-objects'.

An example export / import workflow would look like:

On the computer hosting the source repository:

```bash
# Go to the site directory of the Drupal site that has Islandora pointed to the source repository
cd /var/www/html/drupal/sites/default/

# Given a list of PIDs in a file called pids.txt
drush --user=1 islandora-export-objects --pids-file-dir=/home/yourhome/export --pids-filename=pids.txt  --include-datastreams=TRUE
```

include-datastreams will export the profile data and file objects of all of the exported objects' datastreams. 

The output of exporting the datastreams will look like this:

```bash
bdh_4805      
 - TN.profile.json
 - DC.profile.json
 - RELS-INT.profile.json
 - RELS-INT
 - POLICY.profile.json
 - TN
 - MODS
 - RELS-EXT
 - JP2.profile.json
 - MODS.profile.json
 - RELS-EXT.profile.json
 - OBJ
 - JPG.profile.json
 - TECHMD
 - JPG
 - OBJ.profile.json
 - POLICY
 - JP2
 - TECHMD.profile.json
 - DC
bdh_4805.json 
bdh_4805.xml  
```bash

Move the files to the destination host and then run the following to import:

```bash
drush --user=1 islandora-import-objects --foxml-files-dir=/home/userdir/export --foxml-files-list=/home/userdir/export/bdh_remaining_files.txt --use-exported-datastreams=TRUE
```
