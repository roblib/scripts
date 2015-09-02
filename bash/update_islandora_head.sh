#!/usr/bin/env bash
# we store all islandora modules in .../sites/all/islandora_modules/
# so we pass our islandora sites all directory to this script as a parameter
# usage ./update_islandora_head.sh sites_all_dir where
# sites_all_dir is the full path drupals sites/all (something like /var/www/drupal/sites/all)
[ $# -eq 0 ] && { echo "Usage: $0 path_to_sites_all"; exit 1; }
cd "$1"
pwd
# dump all the drupal databases to the users drush back directory
drush sql-dump @sites --result-file

# get the latest islandora modules
for d in $1/modules/islandora_modules/*/ ; do
    cd "$d"
    pwd
    git checkout 7.x
    git pull origin 7.x
done

#update tuque
cd "$1/libraries/tuque"
pwd
git checkout 1.x
git pull origin 1.x

#update openseadragon
cd "$1/libraries/openseadragon"
pwd
git checkout master
git pull origin master

# run update.php for all sites
cd $1
pwd
drush updb @sites -y