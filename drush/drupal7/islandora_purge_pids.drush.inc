<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 islandora_purge_pids path_to_query.txt TRUE
 * the above would give you an interactive purge
 * if the last parameter is not TRUE then all the pids in the list will be purged without prompting
 * purging an object cannot be undone so use wisely (you have been warned)
 */

//drush hook

function islandora_purge_pids_drush_command() {
  $items = array();

  $items['islandora_purge_pids'] = array(
    'description' => "deletes objects, a list of pids is read from a text file one pid per line.  Please use with caution as purged objects are unrecoverable!
      You will have to use the drush -u switch or you may not have permission to purge some objects.
        Currently relies on the Drupal7 version of Islandora.",
    'arguments' => array(
      'pid_file' => 'a file containing a list of pids, one pid per line',
      'interactive' => 'if TRUE then you will be asked to confirm the purge action for each object'
    ),
    'examples' => array(
      'drush -u 1 islandora_purge_pids path_to_pid_file.txt TRUE',
    ),
    'aliases' => array('islandorapp'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_islandora_purge_pids($pid_file, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($pid_file)) {
    drush_print("Used file $pid_file \n");
  }
  else {
    drush_print("no query file specified");
    return;
  }
  islandora_purge_pids_doAction($pid_file, $interactive);
}

//just a function
function islandora_purge_pids_doAction($pid_file, $interactive) {
  global $user;
  $tuque = islandora_get_tuque_connection($user);
  $repository = $tuque->repository;
  $pids = file($pid_file);

  foreach ($pids as $object) {
    drush_print($object);
  }
  //drush_print("are you sure you want to delete ".count($objects)." objects");
  $num = count($pids);
  if (!drush_confirm(dt('are you sure you want to delete @num objects?', array('@num' => $num)))) {
    // was: return drush_set_error('CORE_SYNC_ABORT', 'Aborting.');
    drush_die('Aborting.');
  }
  if ($interactive == 'TRUE') {
    foreach ($pids as $pid) {
      if (drush_confirm(dt('purge @pid ?', array('@pid' => $pid)))) {
        $repository->purgeObject(trim($pid));
      }
    }
  }
  else {
    foreach ($pids as $pid) {
      $repository->purgeObject(trim($pid));
      drush_print("purged object $pid");
    }
  }
}

?>
