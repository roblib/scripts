<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 islandora_gsearch_update path/to/query.txt gsearch_solr http://localhost:8080/fedoragsearch/rest TRUE
 * the above would give you an interactive update
 * if the last parameter is not TRUE then all the pids in the list will be indexed without prompting
 * 
 */

//drush hook

function islandora_gsearch_update_drush_command() {
  $items = array();

  $items['islandora_gsearch_update'] = array(
    'description' => "updates a gsearch index using a list of pids genereated from an itql query.",
    'arguments' => array(
      'query_file' => 'The path to a plain text file that contains an itql query',
      'index_name' => 'the name of the index to update.',
      'gsearch_url' => 'the url for gsearch, ie. http://localhost/fedoragsearch/rest',
      'interactive' => 'if TRUE then you will be asked to confirm the export for each object'),
    'examples' => array(
      'drush islandora_gsearch_update path/to/query.txt gsearch_solr http://localhost:8080/fedoragsearch/rest TRUE',
    ),
    'aliases' => array('islandoragu'),
      //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_islandora_gsearch_update($query_file, $index_name, $gsearch_url, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($query_file)) {
    drush_print("Using query file " . $query_file . "\n");
    $query = file_get_contents($query_file);
    drush_print("Using Query " . $query);
  }
  else {
    drush_print("no query file specified");
    return;
  }
  if (isset($index_name)) {
    drush_print(" and index name of " . $index_name);
  }
  else {
    drush_print(" no index name specified");
    return;
  }
  if (isset($gsearch_url)) {
    drush_print(" and gsearch url of " . $gsearch_url);
  }
  else {
    drush_print(" no index name specified");
    return;
  }
  islandora_gsearch_update_doAction($query, $index_name, $gsearch_url, $interactive);
}

//just a function
function islandora_gsearch_update_doAction($query, $index_name, $gsearch_url, $interactive) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $results = islandora_gsearch_update_run_query($query);
  $objects = islandora_gsearch_update_sparql_results_as_array($results);
  foreach ($objects as $object) {
    drush_print($object);
  }
  
  $num = count($objects);
  if (!drush_confirm(dt('are you sure you want to index @num objects?', array('@num' => $num)))) {
    // was: return drush_set_error('CORE_SYNC_ABORT', 'Aborting.');
    drush_die('Aborting.');
  }
  if ($interactive == 'TRUE') {
    foreach ($objects as $object) {
      if (drush_confirm(dt('index @pid ?', array('@pid' => $object)))) {
        //?operation=updateIndex&action=fromPid&value=ir%3A2194&indexName=gsearch_solr
        $url = $gsearch_url . "?operation=updateIndex&action=fromPid&value=$object&indexName=$index_name";
        $return_value = drupal_http_request($url);
        drush_print('updated ' . $object . ' ' . $return_value->status_message);
      }
    }
  }
  else {
    foreach ($objects as $object) {
      $url = $gsearch_url . "?operation=updateIndex&action=fromPid&value=$object&indexName=$index_name";
      $return_value = drupal_http_request($url);
      drush_print('updated ' . $object . ' ' . $return_value->status_message);
    }
  }
}

function islandora_gsearch_update_sparql_results_as_array($content) {
  $content = new SimpleXMLElement($content);

  $resultsarray = array();
  foreach ($content->results->result as $result) {
    $resultsarray[] = substr($result->object->attributes()->uri, 12); // Remove 'info:fedora/'.
  }
  return $resultsarray;
}

function islandora_gsearch_update_run_query($query) {
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
