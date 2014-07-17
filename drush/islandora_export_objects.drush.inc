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
      'query_file' => 'The path to a plain text file that contains a list of pids',
      'type' => 'the type of export migrate or archive',
      'interactive' => 'if TRUE then you will be asked to confirm the export for each object'
    ),
    'examples' => array(
      'drush islandora_export_objects /path/to/query.txt archive TRUE',
    ),
    'aliases' => array('islandoraexp'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_islandora_export_objects($query_file, $type, $interactive) {
  drush_print('Current working directory ' . getcwd());
  $export_dir = getcwd();

  if (isset($query_file)) {
    drush_print(" using query file" . $query_file);
  }
  else {
    drush_print(" no query_file found");
    return;
  }
  islandora_export_objects_doAction($query_file, $type, $interactive);
}

/**
 *
 * 
 * @param string $query_file
 * @param string $interactive
 * @return void
 */
function islandora_export_objects_doAction($query_file, $type, $interactive) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  ini_set("auto_detect_line_endings", "1");
  $objects = file($query_file);
  $location = drush_find_tmp();
  drush_print('files are located in ' . $location);
  //drush_print($objects);
  if (!isset($objects)) {
    drush_die('no objects found');
    return;
  }
  foreach ($objects as $object) {
    drush_print($object);
  }
  $num = count($objects);
  if (!drush_confirm(dt('are you sure you want to export @num objects?', array('@num' => $num)))) {
    // was: return drush_set_error('CORE_SYNC_ABORT', 'Aborting.');
    drush_die('Aborting.');
  }


  if ($interactive == 'TRUE') {
    foreach ($objects as $object) {
      if (drush_confirm(dt('export this  @pid ?', array('@pid' => $object)))) {
        $item = new Fedora_Item(trim($object));
        $foxml = $item->export_as_foxml($type);
        $file_name = str_replace(':', '-', $item->pid);
        $file_name = drush_find_tmp() . '/drush_export/' . $file_name . '.foxml';
        drush_print('saving file to ' . $file_name);
        file_put_contents($file_name, $foxml);
        if(method_exists($item, 'forget')){
          $item->forget(); //on some vre2 sites this function does not exist
        }
        unset($item);
      }
    }
  }
  else {
    foreach ($objects as $object) {
      $item = new Fedora_Item(trim($object));
      $foxml = $item->export_as_foxml($type);
      $file_name = str_replace(':', '-', $item->pid);
      $file_name = drush_find_tmp() . '/drush_export/' . $file_name . '.foxml';
      drush_print('saving file to ' . $file_name);
      file_put_contents($file_name, $foxml);
      if(method_exists($item, 'forget')){
        $item->forget(); //on some vre2 sites this function does not exist
      }
      unset($item);
    }
  }
  drush_print('files are located in ' . $location);
}

/**
 *
 * @param strings $query
 * @return string
 */
function islandora_export_objects_run_query($query) {
  $results = islandora_export_run_query($query); //function in si_batch_ingest.drush.inc
  if (isset($results)) {//still doesn't tell us if these are valid results
    return $results;
  }
  else {
    drush_print('Error executing resource index query');
    return NULL;
  }
}

function islandora_export_sparql_results_as_array($content) {
  $content = new SimpleXMLElement($content);

  $resultsarray = array();
  foreach ($content->results->result as $result) {
    $resultsarray[] = substr($result->object->attributes()->uri, 12); // Remove 'info:fedora/'.
  }
  return $resultsarray;
}

function islandora_export_run_query($query) {
  module_load_include('inc', 'fedora_repository', 'CollectionClass');
  $cc = new CollectionClass();
  $results = $cc->getRelatedItems(NULL, $query, 1000000);
  if (isset($results)) {//still doesn't tell us if these are valid results
    return $results;
  }
  else {
    drush_print('Error get related items, relationships cannot be updated');
  }
}

?>
