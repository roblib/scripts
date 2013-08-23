<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 
 * 
 */

//drush hook

function islandora_update_broken_page_numbers_drush_command() {
  $items = array();

  $items['update_pages'] = array(
    'description' => "Reads a csv file first column is pid.  
      Currently requires Drupal7 Islandora with Tuque.  
      Each pid in the txt/csv file should represent a book or newspaper (the script looks up the pages base on the parent newspaper or book).  
      This is pretty specific to UPEI's newspapers right now)",
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
 * calls the doAction function
 * 
 * @param string $query_file
 *  path to a text file that contains an itql query
 * @param boolean $interactive
 * 
 */
function drush_islandora_update_page_numbers_update_pages($csv_file, $interactive) {
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
 * Iterates through all the objects returned by the itql query and updates the mods stream
 * 
 * @param string $csv_file
 * @param boolean $interactive
 * @param string $action
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
  $pages = islandora_basic_collection_get_objects($parent_object);
  unset($parent_object);
  //limit actions while testing
  //$count = 0;
  foreach ($pages as $page) {
    //var_dump($page);
    //$count++;
   // if($count > 1){
   //   return;
   // }
    $page_number = trim(substr($page['title']['value'],4));
    //var_dump($page_number);
    //we make a big assumption here THAT every page has a title like "Page 3"
    //for UPEI the titles are Page 1, Page 2 etc so this works but will not work 
    //if the titles don't sort as expected.
    drush_print("processing rels of page # " . $page['object']['value']);
    $page_object = islandora_object_load($page['object']['value']);
    //$pid = $page_object->id;
    $page_object->relationships->remove('http://islandora.ca/ontology/relsext#', 'isSequenceNumber');
    $page_object->relationships->remove("http://islandora.ca/ontology/relsext#", 'isPageNumber');
    $page_object->relationships->add("http://islandora.ca/ontology/relsext#", 'isPageNumber', $page_number, TRUE);
    $page_object->relationships->add("http://islandora.ca/ontology/relsext#", 'isSequenceNumber', $page_number, TRUE);
    
    unset($page_object);
  }
}


?>