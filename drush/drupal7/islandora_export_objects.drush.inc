<?php

/*
 * This file should be put in the drush/commands/islandora directory of your drush install
 * (you may have to create the islandora directory
 * 
 * updates xacml policies on all excepts specified by an itql query. 
 * 
 * @todo: add a datastream parameter then we could pass a file and update either collection_policy
 * xacml policy or any other dsid that is xml and needs to be updated across a group of objects.
 */

//drush hook

function islandora_export_objects_drush_command() {
  $items = array();

  $items['islandora_export_objects'] = array(
    'description' => "exports foxml for each object returned by a query, files will be located in the sites temp dir.
      Currently relies on the Drupal 6 version of Islandora",
    'arguments' => array(
      'query_file' => 'The path to a plain text file that contains an itql query',
      'interactive' => 'if TRUE then you will be asked to confirm the export for each object'
    ),
    'examples' => array(
      'drush islandora_export_objects /path/to/query.txt TRUE',
    ),
    'aliases' => array('islandoraexp'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_islandora_export_objects($query_file, $interactive) {
  drush_print('Current working directory ' . getcwd());
  $export_dir = getcwd();

  if (isset($query_file)) {
    drush_print(" using query file" . $query_file);
  }
  else {
    drush_print(" no query_file found");
    return;
  }
  islandora_export_objects_doAction($query_file, $interactive);
}

/**
 *
 * 
 * @param string $query_file
 * @param string $interactive
 * @return void
 */
function islandora_export_objects_doAction($query_file, $interactive) {
  //module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  ini_set("auto_detect_line_endings", "1");
  $objects = file($query_file);
  $location = drush_find_tmp();
  drush_print('files are located in ' . $location);

  if (!isset($objects)) {
    drush_die('no objects found');
    return;
  }

  global $user;
  $tuque = islandora_get_tuque_connection($user);
  $repository = $tuque->repository;

  foreach ($objects as $object) {
    drush_print(trim($object));
  }

  $num = count($objects);
  if (!drush_confirm(dt('are you sure you want to export @num objects?', array('@num' => $num)))) {
    drush_die('Aborting.');
  }


  if ($interactive == 'TRUE') {
    foreach ($objects as $pid) {
      $pid = trim($pid);
      if (drush_confirm(dt('export this  @pid ?', array('@pid' => $pid)))) {
        $foxml = $repository->api->m->export($pid, array('context' => 'migrate'));

        $file_name = str_replace(':', '-', $pid);
        //TODO create the drush_export directory if it does not exist
        $file_name = drush_find_tmp() . '/drush_export/' . $file_name . '.foxml';
        drush_print('saving file to ' . $file_name);
        file_put_contents($file_name, $foxml);
      }
    }
  }
  else {
    foreach ($objects as $pid) {
      $pid = trim($pid);
      $foxml = $repository->api->m->export($pid, array('context' => 'migrate'));
      $file_name = str_replace(':', '-', $pid);
      $file_name = drush_find_tmp() . '/drush_export/' . $file_name . '.foxml';
      drush_print('saving file to ' . $file_name);
      file_put_contents($file_name, $foxml);
    }
  }
  drush_print('files are located in ' . $location);
}

/**
 * returns an array of pids
 * @todo pull this up into an api
 * @param SimpleXMLElement $content
 * @return array
 */
function islandora_export_sparql_results_as_array($results) {
    $resultsarray = array();
    foreach($results as $result){
        // var_dump($result);
        $resultsarray[] = $result['object']['value'];
    }
    return $resultsarray;
}


?>
