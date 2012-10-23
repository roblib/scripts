<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 
 * 
 */

//drush hook

function islandora_update_departments_solr_drush_command() {
  $items = array();

  $items['islandora_update_departments_solr_drush'] = array(
    'description' => "Reads a solr result set and updates the mods:identifier[@type='u2'] field.  This is specific to islandscholar",
    'arguments' => array(
      'solr_file' => 'The path to a csv file that contains columns as described in the description.',
      'datastream_id' => 'the id of the datastream to load',
      'interactive' => 'if TRUE then you will be asked to confirm the update for each object',
    ),
    'examples' => array(
      'drush -u 1 --uri=http://137.149.200.19/scholardev islandoraudptsolr /var/www/html/drupal/sites/137.149.200.19.scholardev/files/solr_results.xml MODS TRUE',
    ),
    'aliases' => array('islandoraudptsolr'),
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
function drush_islandora_update_departments_solr_drush($solr_file, $dsid, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($solr_file)) {
    drush_print(" using solr  file" . $csv_file);
  }
  else {
    drush_print(" no solr file found");
    return;
  }
  if (isset($dsid)) {
    drush_print(" using dsid " . $dsid);
  }
  else {
    drush_print("no datastream id specified");
    return;
  }

  islandora_update_from_solr_go($solr_file, $dsid, $interactive);
}

/**
 * Iterates through all the objects returned by the itql query and updates the mods stream
 * 
 * @param string $query_file
 * @param string $substitution_text
 * @param boolean $interactive
 * 
 */
function islandora_update_from_solr_go($solr_file, $dsid, $interactive = FALSE) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  ini_set("auto_detect_line_endings", "1");
  $query_results = $query = file_get_contents($solr_file);
  $line_number = 0;
  $xml = new SimpleXMLElement($query);
  $docs = $xml->xpath('//doc');
  foreach ($docs as $doc) {
    $pid = (string) $doc->str;
    $arr = $doc->arr;
    $depts = array();
    foreach ($arr->str as $dept) {
      $depts[] = (string) $dept;
    }
    if ($interactive == 'TRUE') {
      if (drush_confirm(dt('update this @pid ?', array('@pid' => $pid)))) {
        islandora_update_department($pid, $dsid, $depts);
        drush_print("processed $pid # " . $line_number++);
        if (($line_number % 500) == 0) {
          drush_print('resting');
          sleep(3); //rest for a few seconds so fedora doesn't get overwhelmed
        }
      }
    }
    else {
      islandora_update_department($pid, $dsid, $depts);
      drush_print("processed $pid # " . $line_number++);
      if (($line_number % 500) == 0) {
        drush_print('resting');
        sleep(10); //rest for a few seconds so fedora doesn't get overwhelmed
      }
    }
  }
}

function islandora_update_department($pid, $dsid, $depts) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  if (!isset($depts)) {
    islandora_update_write_log(dt('empty line'), 'islandora_update_department.log', 'error');
    return;
  }
  $item = new Fedora_Item($pid);
  if (!$item->exists()) {
    islandora_update_write_log(dt('error loading object %pid', array('%pid' => $pid)), 'islandora_update_department.log', 'error');
    $item->forget();
    return;
  }

  $metadata = $item->get_datastream_dissemination($dsid);
  $sxml = simplexml_load_string($metadata);
  if (!$sxml) {
    islandora_update_write_log(dt('error processing %pid', array('%pid' => $pid)), 'islandora_update_department.log', 'error');
    $item->forget();
    return;
  }
  $sxml->registerXPathNamespace('ns', 'http://www.loc.gov/mods/v3');


  $elements = $sxml->xpath('//ns:identifier[@type="u2"]');
  //remove existing departments
  foreach ($elements as $element) {
    $dom = dom_import_simplexml($element);
    $dom->parentNode->removeChild($dom);
  }
  // add new ones from file
  foreach ($depts as $dept) {
    $new_node = $sxml->addChild('identifier', $dept, 'http://www.loc.gov/mods/v3');
    $new_node->addAttribute('type', 'u2');
  }

  $content = $sxml->asXML();
  if (isset($content)) {
    $return_value = $item->purge_datastream($dsid, NULL, NULL, 'removing datastream using update departments script');
    $value = $item->add_datastream_from_string($content, $dsid, $dsid, 'text/xml', 'X', 'Adding datastream using update departments script');
    //$value = $item->modify_datastream($content, $dsid, $dsid, 'text/xml', $force = FALSE, $logMessage = 'Modified by Islandora API using islandora_update_from_csv drush script', TRUE);
  }
  if (isset($value)) {
    islandora_update_write_log(dt('successfully updated %dsid for %pid', array('%dsid' => $dsid, '%pid' => $pid)), 'islandora_update_department.log');
  }
  else {
    islandora_update_write_log(dt('error updating %dsid for %pid', array('%dsid' => $dsid, '%pid' => $pid)), 'islandora_update_department.log', 'error');
  }
  $item->forget();
  time_nanosleep(0, 250000000);
}

?>