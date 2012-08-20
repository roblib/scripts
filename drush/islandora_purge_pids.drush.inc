<?php

/*
 *This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 islandora_purge_pids collection:pid isMemberOf TRUE
 * the above would give you an interactive purge
 * if the last parameter is not TRUE then all the pids in the list will be purged without prompting
 * purging an object cannot be undone so use wisely (you have been warned)
 */

//drush hook

function islandora_purge_pids_drush_command() {
  $items = array();

  $items['islandora_purge_pids'] = array(
    'description' => "deletes all objects in a collection.  Please use with caution as purged objects are unrecoverable!  You will have to use the drush -u switch or you may not have permission to purge some objects",
    'arguments' => array(
      'collection_pid' => 'The pid of the collection object',
      'relationship' => 'The relationship the objects have to the collection object.  For example isMemberOf is a common relationship.',
      'interactive' => 'if TRUE then you will be asked to confirm the purge action for each object'
    ),
    'examples' => array(
      'drush islandora_purge_pids islandora:collection isMemberOfCollection',
    ),
    'aliases' => array('islandorapp'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_islandora_purge_pids($collection_pid, $relationship, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($collection_pid)) {
    drush_print("Used collection pid of " . $collection_pid . "\n");
  }
  else {
    drush_print("no collection pid specified");
    return;
  }
  if (isset($relationship)) {
    drush_print(" and relationship of " . $relationship);
  }
  else {
    drush_print(" no relationship specified");
    return;
  }
  islandora_purge_pids_doAction($collection_pid, $relationship, $interactive);
}

//just a function
function islandora_purge_pids_doAction($collection_pid, $relationship, $interactive) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $objects = get_related_items_as_array($collection_pid, $relationship);
  foreach ($objects as $object) {
    drush_print($object);
  }
  //drush_print("are you sure you want to delete ".count($objects)." objects");
  $num = count($objects);
  if (!drush_confirm(dt('are you sure you want to delete @num objects?', array('@num' => $num)))) {
    // was: return drush_set_error('CORE_SYNC_ABORT', 'Aborting.');
    drush_die('Aborting.');
  }
  if ($interactive == 'TRUE') {
    foreach ($objects as $object) {
      if (drush_confirm(dt('purge @pid ?', array('@pid' => $object)))) {
        $item = new Fedora_Item($object);
        $item->purge('purged using islandorapp drush script');
      }
    }
  }
  else {
    foreach ($objects as $object) {
      $item = new Fedora_Item($object);
      $item->purge('purged using islandorapp drush script no confirm');
    }
  }
}

?>
