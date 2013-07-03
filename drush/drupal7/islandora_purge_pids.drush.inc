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
    'description' => "deletes all objects in a collection.  Please use with caution as purged objects are unrecoverable!  
      You will have to use the drush -u switch or you may not have permission to purge some objects.
        Currently relies on the Drupal6 version of Islandora.",
    'arguments' => array(
      'query_file' => 'The pid of the collection object',
      'interactive' => 'if TRUE then you will be asked to confirm the purge action for each object'
    ),
    'examples' => array(
      'drush islandora_purge_pids path_to_query.txt TRUE',
    ),
    'aliases' => array('islandorapp'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_islandora_purge_pids($query_file, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($query_file)) {
    drush_print("Used file $query_file \n");
  }
  else {
    drush_print("no query file specified");
    return;
  }
  islandora_purge_pids_doAction($query_file, $interactive);
}

//just a function
function islandora_purge_pids_doAction($query_file, $interactive) {
  global $user;
  $tuque = islandora_get_tuque_connection($user);
  $repository = $tuque->repository;
  $query = file_get_contents($query_file);
  $results = $repository->ri->itqlQuery($query);
  $objects = islandora_purge_pids_sparql_results_as_array($results);
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
    foreach ($objects as $pid) {
      if (drush_confirm(dt('purge @pid ?', array('@pid' => $pid)))) {
        $repository->purgeObject($pid);
      }
    }
  }
  else {
    foreach ($objects as $pid) {
      $repository->puregeObject($pid);
    }
  }
}

/**
 * returns an array of pids 
 * @todo pull this up into an api
 * @param SimpleXMLElement $content
 * @return array
 */
function islandora_purge_pids_sparql_results_as_array($results) {
  $resultsarray = array();
  foreach($results as $result){
   // var_dump($result);
    $resultsarray[] = $result['object']['value'];
  }
  return $resultsarray;
}

?>
