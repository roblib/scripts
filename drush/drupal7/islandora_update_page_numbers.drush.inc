<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 
 * 
 */

//drush hook

function islandora_update_page_numbers_drush_command() {
  $items = array();

  $items['islandora_update_page_numbers'] = array(
    'description' => "Reads a file containing a list of pids.  One pid per line.
      Currently requires Drupal7 Islandora with Tuque.  
      Each pid in the txt/csv file should represent a book or newspaper.",
    'arguments' => array(
      'csv_file' => 'The path to a csv file that contains columns as described in the description.',
      'interactive' => 'if TRUE then you will be asked to confirm the update for each object',
    ),
    'examples' => array(
      'drush -u 1 --uri=http://137.149.200.19/scholardev islandoraup /var/www/html/drupal/sites/137.149.200.19.scholardev/files/csv.txt TRUE',
    ),
    'aliases' => array('islandoraup'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

/**
 * This is the drush command specified in the array create by the drush entry point.
 * 
 * This function checks to make sure parameters are supplied and if everything is ok
 * calls the go function
 * 
 * @param string $query_file
 *  path to a text file that contains an itql query
 * @param boolean $interactive
 * 
 */
function drush_islandora_update_page_numbers($csv_file, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($csv_file)) {
    drush_print(" using csv file" . $csv_file);
  }
  else {
    drush_print(" no csv file found");
    return;
  }
  islandora_update_pages_go($csv_file, $interactive);
}

/**
 * Reads each line of a file for a pid.
 *
 * Then calls the update rels functioin
 * 
 * @param string $csv_file
 *   A file with a list of pids one pid per line
 * @param boolean $interactive
 *   If true the script will prompt before updating the pages in a book.  This
 * only prompts at the book level not the page level.
 * 
 */
function islandora_update_pages_go($csv_file, $interactive = FALSE) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  ini_set("auto_detect_line_endings", "1");

  $line_number = 0;
  $lines = file($csv_file);
  $count = 0;
  foreach ($lines as $pid) {
    $count++;
    $pid = trim($pid);
    if (!empty($pid)) {
      
      
        if ($interactive == 'TRUE') {
          if (drush_confirm(dt('update this @pid ?', array('@pid' => $pid)))) {
            //$item = new Fedora_Item($object);
            islandora_drush_update_page_rels($pid);
            drush_print("processed rels of $pid # " . ++$line_number);
          }
        }
        else {
          islandora_drush_update_page_rels($pid);
          drush_print("processed rels of $pid # " . ++$line_number);
        }
      
      if($count > 100){
        time_nanosleep(0, 500000000);
        $count = 0;
      }
    }
  }
}





function islandora_drush_update_page_rels($parent_pid) {
  $parent_object = islandora_object_load($parent_pid);
  if(empty($parent_object)){
    drush_print("no object found for pid $parent_pid");
    return;
  }
  module_load_include('inc', 'islandora_basic_collection', 'includes/utilities');
  $pages = islandora_basic_collection_get_child_pids($parent_object);

  //limit actions while testing
  //$count = 0;
  foreach ($pages as $page) {


    drush_print("processing rels of page # " . $page);
    $page_object = islandora_object_load($page);
    $page_number_arr = $page_object->relationships->get("info:islandora/islandora-system:def/pageinfo#", 'isPageNumber');
    if(empty($page_number_arr)){
      // This script may have already been run and updated the isPageNumber namespace so we'll try with the new namespace
      $page_number_arr = $page_object->relationships->get("http://islandora.ca/ontology/relsext#", 'isPageNumber');
    }
    $page_number = $page_number_arr[0]['object']['value'];
    drush_print("processing page number $page_number ");

    //remove old relationships that may not be using literals and old namespaces
    //if (drush_confirm(dt('update this @pid ?', array('@pid' => $page)))) {
      $page_object->relationships->remove('info:islandora/islandora-system:def/pageinfo#', 'isSequenceNumber');
      $page_object->relationships->remove("info:islandora/islandora-system:def/pageinfo#", 'isPageNumber');
      $page_object->relationships->remove("info:islandora/islandora-system:def/pageinfo#", 'isPageOf');
      $page_object->relationships->remove('http://islandora.ca/ontology/relsext#', 'isSequenceNumber');
      $page_object->relationships->remove("http://islandora.ca/ontology/relsext#", 'isPageNumber');
      $page_object->relationships->remove("http://islandora.ca/ontology/relsext#", 'isPageOf');
      $page_object->relationships->remove("http://islandora.ca/ontology/relsext#", 'hasLanguage');
      $page_object->relationships->add("http://islandora.ca/ontology/relsext#", 'isPageNumber', $page_number, TRUE);
      $page_object->relationships->add("http://islandora.ca/ontology/relsext#", 'isSequenceNumber', $page_number, TRUE);
      $page_object->relationships->add("http://islandora.ca/ontology/relsext#", "hasLanguage", "eng", TRUE);
      $page_object->relationships->add("http://islandora.ca/ontology/relsext#", "isPageOf", $parent_pid,false);
    //}
    unset($parent_object);
    unset($page_object);
  }
}


?>