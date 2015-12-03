#!/usr/bin/env bash
# we store all islandora modules in .../sites/all/islandora_modules/
# so we pass our islandora sites all directory to this script as a parameter
# usage ./update_islandora_head.sh sites_all_dir where
# sites_all_dir is the full path drupals sites/all (something like /var/www/drupal/sites/all)
[ $# -eq 0 ] && { echo "Usage: $0 path_to_sites_all"; exit 1; }
cd "$1"
pwd
# backup the islandora_modules and libraries directory
# Since we are upating to head from a previous head update and we are updating several repos it is easier to back up the dirs than to track all the commits we would need to revert to
tempDir=$(mktemp -d /tmp/island.XXXXXXXXXX) || { echo "Failed to create temp directory"; exit 1; }
echo $tempDir
(cp -r $1/modules/islandora_modules $tempDir) || { echo "ERROR Failed to backup islandora_modules directory"; exit 1; }
echo "Successfully backed up islandora_modules directory to $tempDir"
(cp -r $1/libraries/tuque $tempDir) || { echo "ERROR Failed to backup tuque library directory"; exit 1; }
echo "Successfully backed up tuque library to $tempDir"
(cp -r $1/libraries/openseadragon $tempDir) || { echo "ERROR Failed to backup openseadragon directory"; exit 1; }
echo "Successfully backed up openseadragon library to $tempDir"
# put the sites in maintanence mode
drush @sites vset --yes maintenance_mode 1
# dump all the drupal databases to the users drush back directory
drush @sites sql-dump --result-file --gzip --yes
# get the latest islandora modules
for d in $1/modules/islandora_modules/*/ ; do
    cd "$d"
    pwd
    git checkout 7.x
    # in case we have modified files remove them
    git checkout -- .
    git pull 
done

#update tuque
cd "$1/libraries/tuque"
pwd
git checkout 1.x
# in case we have modified files remove them
git checkout -- .
git pull

#update openseadragon
cd "$1/libraries/openseadragon"
pwd
git checkout master
# in case we have modified files remove them
git checkout -- .
git pull 

# run update.php for all sites
cd $1
pwd
drush @sites updb -y
drush @sites vset --yes maintenance_mode 0
