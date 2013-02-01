<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 
 * 
 */

//drush hook

function islandora_update_from_csv_drush_command() {
  $items = array();

  $items['islandora_update_from_csv_drush'] = array(
    'description' => "Reads a csv file first column is pid 2nd is username and third column is department.  This is specific to islandscholar UPEI,
       and requires Drupal6 version of Islandora",
    'arguments' => array(
      'csv_file' => 'The path to a csv file that contains columns as described in the description.',
      'datastream_id' => 'the id of the datastream to load',
      'interactive' => 'if TRUE then you will be asked to confirm the update for each object',
    ),
    'examples' => array(
      'drush -u 1 --uri=http://137.149.200.19/scholardev islandoraucsv /var/www/html/drupal/sites/137.149.200.19.scholardev/files/csv.txt MODS TRUE',
    ),
    'aliases' => array('islandoraucsv'),
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
function drush_islandora_update_from_csv_drush($csv_file, $dsid, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($csv_file)) {
    drush_print(" using csv file" . $csv_file);
  }
  else {
    drush_print(" no csv file found");
    return;
  }
  if (isset($dsid)) {
    drush_print(" using dsid " . $dsid);
  }
  else {
    drush_print("no datastream id specified");
    return;
  }

  islandora_update_csv_go($csv_file, $dsid, $interactive);
}

/**
 * Iterates through all the objects returned by the itql query and updates the mods stream
 * 
 * @param string $query_file
 * @param string $substitution_text
 * @param boolean $interactive
 * 
 */
function islandora_update_csv_go($csv_file, $dsid, $interactive = FALSE) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  ini_set("auto_detect_line_endings", "1");
  $fh = fopen($csv_file, 'r');
  $line_number = 0;

  while (($data = fgetcsv($fh)) !== FALSE) {
    if ($line_number++ > 0) {
      $pid = $data[0];
      $u1 = $data[2];
      $u2 = $data[3];
      if ($interactive == 'TRUE') {
        if (drush_confirm(dt('update this @pid ?', array('@pid' => $pid)))) {
          //$item = new Fedora_Item($object);
          islandora_update_datastream_from_csv($pid, $dsid, $data);
          drush_print("processed $pid # " . $line_number);
        }
      }
      else {
        islandora_update_datastream_from_csv($pid, $dsid, $data);
        drush_print("processed $pid # " . $line_number);
      }
    }
  }
  fclose($fh);
}

/**
 * update all the objects
 * 
 * @param string $pid
 * 
 */
function islandora_update_datastream_from_csv($pid, $dsid, $data) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  if (!isset($data)) {
    islandora_update_write_log(dt('empty line'), 'islandora_update_from_csv.log', 'error');
    return;
  }
  $item = new Fedora_Item($pid);
  if (!$item->exists()) {
    islandora_update_write_log(dt('error loading object %pid', array('%pid' => $pid)), 'islandora_update_from_csv.log', 'error');
    $item->forget();
    return;
  }

  $metadata = $item->get_datastream_dissemination($dsid);
  $sxml = simplexml_load_string($metadata);
  if (!$sxml) {
    islandora_update_write_log(dt('error processing %pid', array('%pid' => $pid)), 'islandora_update_from_csv.log', 'error');
    $item->forget();
    return;
  }

  $sxml->registerXPathNamespace('ns', 'http://www.loc.gov/mods/v3');
  $elements = $sxml->xpath('//ns:identifier[@type="u1"]');
  $updated = FALSE;
  $exists = FALSE;
  $new_str = $data[3];
  foreach ($elements as $node) {
    $original_str = (string) $node;
    islandora_update_write_log(dt('processing %pid u1 field', array('%pid' => $pid)), 'islandora_update_from_csv.log');
    islandora_update_write_log(dt('original string = %org_string new string = %new_string', array('%org_string' => $original_str, '%new_string' => $new_str)), 'islandora_update_from_csv.log');
    if (trim($new_str) == trim($original_str)) {
      $exists = TRUE;
    }
  }
  if (!$exists && !empty($new_str)) {
    $new_node = $sxml->addChild('identifier', $new_str, 'http://www.loc.gov/mods/v3');
    $new_node->addAttribute('type', 'u1');
    islandora_update_write_log(dt('updated u1 field in %dsid for %pid', array('%dsid' => $dsid, '%pid' => $pid)), 'islandora_update_from_csv.log');
    $updated = TRUE;
  }

  //u2 = departments
  $elements = $sxml->xpath('//ns:identifier[@type="u2"]');
   
  $new_strs = explode(';', $data[4]);
  $original_strs = array();
  $arr = array();
  foreach ($elements as $node) {
    $original_strs[] = (string) $node;
  }
   islandora_update_write_log(dt('processing %pid u2 field', array('%pid' => $pid)), 'islandora_update_from_csv.log');
    //islandora_update_write_log(dt('original string = %org_string new string = %new_string', array('%org_string' => $original_str, '%new_string' => $new_str)), 'islandora_update_from_csv.log');
   if(!empty($original_strs)){
    $arr = array_diff($new_strs, $original_strs);    
   } else {
     $arr = $new_strs;
   }
   
  foreach ($arr as $new_str) {
    if (!empty($new_str)) {
      $new_node = $sxml->addChild('identifier', $new_str, 'http://www.loc.gov/mods/v3');
      $new_node->addAttribute('type', 'u2');
      islandora_update_write_log(dt('updated u2 field in %dsid for %pid and %value', array('%dsid' => $dsid, '%pid' => $pid,'%value' => $new_str)), 'islandora_update_from_csv.log');
      $updated = TRUE;
    }
  }

  if ($updated) {
    $content = $sxml->asXML();
    if (isset($content))
      $value = $item->modify_datastream($content, $dsid, $dsid, 'text/xml', $force = FALSE, $logMessage = 'Modified by Islandora API using islandora_update_from_csv drush script', TRUE);
    if (isset($value)) {
      islandora_update_write_log(dt('successfully updated %dsid for %pid', array('%dsid' => $dsid, '%pid' => $pid)), 'islandora_update_from_csv.log');
    }
    else {
      islandora_update_write_log(dt('error updating %dsid for %pid', array('%dsid' => $dsid, '%pid' => $pid)), 'islandora_update_from_csv.log', 'error');
    }   
    time_nanosleep(0, 250000000);
  }
    $item->forget();
}

?>