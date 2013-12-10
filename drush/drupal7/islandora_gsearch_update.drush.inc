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
    'description' => "updates a gsearch index using a list of pids read from a text file, should work with islandora 6.x or islandora 7.x.",
    'arguments' => array(
      'pidlist_file' => 'The path to a plain text file that contains a list of pids one pid per line',
      'index_name' => 'the name of the index to update.',
      'gsearch_url' => 'the url for gsearch, ie. http://localhost/fedoragsearch/rest',
      'interactive' => 'if TRUE then you will be asked to confirm the export for each object'
    ),
    'examples' => array(
      'drush islandora_gsearch_update path/to/pidlist.txt gsearch_solr http://localhost:8080/fedoragsearch/rest TRUE',
    ),
    'aliases' => array('islandoragu'),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_islandora_gsearch_update($pidlist_file, $index_name, $gsearch_url,
                                        $interactive) {
  drush_print('Current working directory ' . getcwd());
  $objects = NULL;
  if (isset($pidlist_file)) {
    drush_print("Using PID file " . $pidlist_file . "\n");
    $objects = file($pidlist_file);
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

  islandora_gsearch_update_doAction($objects, $index_name, $gsearch_url,
     $interactive);
}

//just a function
function islandora_gsearch_update_doAction($objects, $index_name, $gsearch_url,
                                           $interactive) {

  foreach ($objects as $object) {
    drush_print($object);
  }
  $num = count($objects);
  if (!drush_confirm(dt('are you sure you want to index @num objects?', array('@num' => $num)))) {
    drush_die('Aborting.');
  }
  if ($interactive == 'TRUE') {
    foreach ($objects as $object) {
      $pid = trim($object);
      if (drush_confirm(dt('index @pid ?', array('@pid' => $object)))) {
        //?operation=updateIndex&action=fromPid&value=ir%3A2194&indexName=gsearch_solr
        $url = $gsearch_url . "?operation=updateIndex&action=fromPid&value=$pid&indexName=$index_name";
        $return_value = drupal_http_request($url);
        drush_print(dt('updated %object using url %url with a return value of %return_value', array(
            '%object' => $object, '%url' => $url, '%return_value' => $return_value->status_message)));
        //drush_print($url);
      }
    }
  }
  else {
    foreach ($objects as $object) {
      $pid = trim($object);
      $url = $gsearch_url . "?operation=updateIndex&action=fromPid&value=$pid&indexName=$index_name";
      $return_value = drupal_http_request($url);
      drush_print(dt('updated %object using url %url with a return value of %return_value', array(
        '%object' => $object, '%url' => $url, '%return_value' => $return_value->status_message)));
    }
  }
}

?>
