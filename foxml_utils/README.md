# Islandora Repository Collection Migration Guide and Utilities

For moving collections from server-to-server, for Islandora running on Drupal 7 and Feodra 3.8. Migrating collections to Fedora 4 is covered in teh official DuraSpace documentation. This guide and accompanying scripts were created in the course of migrating the contents of the http://bowingdownhome.ca/ collection from a dedicated server to a central repository.

## Get list of un-migrated objects

You can use Solr to get a list of PIDs from the source and destination repositories if they have Solr properly installed and connected to Islandora:
```bash
curl "http://hp1.islandarchives.ca:8080/solr/collection1/select?q=PID%3Abdh*&fl=PID&wt=csv&indent=true&rows=30000&sort=PID+asc" > bdh_hp1.csv
 curl "http://bowingdownhome.ca:8080/solr/collection1/select?q=PID%3Abdh*&fl=PID&wt=csv&indent=true&rows=30000&sort=PID+asc" > bdh_bdh.csv
```

### Get list of PIDs that are missing from one of the two sets of PIDs:
```bash
comm -23 bdh*sorted* # Remember this command only works if both lists are sorted.
```

Steps to export, repair and then import objects from a repository.

Put a list of PIDs to export into a text file, run export.py on the source repository server:

```bash
python3 /path/to/export.py --password=<password> --pids-file=/path/to/[pids.txt] --export-type=migrate
```

Often Fedora is installed with "localhost" left as the hostname configuration. This needs to be changed to the server's hostname so that datastreams can be retrieved during import.

You can fix this hostname with the following command:

```bash
$ perl -i -pe 's/localhost:8080/[source-hostname]:8080/g' ./*.xml
```

Replace [source-hostname] with the actual hostname of the source repository's server.

(Tecnnichal side note: I'm using Perl instead of sed here because sed command line syntax is slightly different between GNU (Linux) and BSD (macos) versions.)

## Datastream cleanup and repair

For Bowing Down Home I've noticed many objects with datastream version elements in the FOXML that link to binaries that are not found in low level storage, which halts the import process. This seems to be an effect of generating derivative datastreams with via Taverna.

An import will fail if even an older version of a datastream's binary URL returns an HTTP error response code. Since these are all derivative datastreams it was decided the easiest way to deal with the problem was to delete their references from the FOXML before importing.

You can run the following script to do two things:

1. Generate a set of FOXML files in a subdirectory that have references to unreachable datastreams stripped out of the FOXML. 
2. Generate a report in a CSV file listing:
    1. The object with missing datastream's PID and the datastream's DSID
    2. If the missing datastream is the most recent one, indicating that the object should get manual attention
    3. If the datastream has no good versions of the affected datastream requiting manual regeneration of derivatives if the datastream is derivative. If the missing datastream is not a derivative datastream the data may be lost.

```bash
python3 /path/to/foxml_utils/foxml_invalid_datastream_cleaner.py --directory=.
```

Where --directory indicates a directory with the exported FOXML files.

Affected files will be created in a subdirectory named 'clean'.

## Importing

There are a few ways to import FOXML files, one is the use the import command I recently added to the islandora_datastream_crud project. drush islandora_datastream_crud_ingest_foxml_files  --foxml_dir=/home/aoneill/clean --fedora_pass=sqr1j3rky --fedora_url=http://hp1.islandarchives.ca:8080/fedora --fedora_user=fedoraAdmin

Install the Drupal 7-specific Drush commands form this 'scripts' repository by creating a shortcut in your home directory under the .drush folder.
```bash
cmtctest:foxml_utils aoneill$ cd ~/.drush/
cmtctest:.drush aoneill$ ls -l
total 0
drwxr-xr-x  5 aoneill  staff  160 20 Apr 14:07 cache
lrwxr-xr-x  1 aoneill  staff   44 13 Apr 10:59 islandora_datastream_crud -> /Users/aoneill/dev/islandora_datastream_crud
lrwxr-xr-x  1 aoneill  staff   41  4 Apr 11:08 scripts -> /Users/aoneill/dev/scripts/drush/drupal7/
```
Then run an ingest command by running:
```bash
drush --user=1 islandora-ingest-foxml-files  --foxml-dir=/Users/aoneill/tmp/export
```

Use drush help islandora-ingest-foxml-files to see other options, including how to override the Drupal filter and use fedoraAdmin credentials directly.
