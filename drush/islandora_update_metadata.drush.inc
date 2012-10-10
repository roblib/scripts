<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 
 *
 * 
 */

//drush hook

function islandora_update_metadata_drush_command() {
  $items = array();

  $items['islandora_update_metadata'] = array(
    'description' => "scans the MODS datastream for fields with certain values and corrects known errors",
    'arguments' => array(
      'query_file' => 'The path to a plain text file that contains an itql query.  This query should return a list of pids.',
      'substitution_text_file' => 'A text file that contains the values to be replaced and the values to update it with.
          This file should be formatted with each entry on a line of by itself such as "incorrect text ~ correct text" eg. "Path Micro ~ Pathology and Microbiology".',
      'datastream_id' => 'the id of the datastream to load',
      'xpath' => 'An xpath statement to select the elements with the incorrect text eg. "//ns:identifier[@type=\'u2\']".  The double quotes are required in this example',
      'namespace' => 'the xml namespace.  http://www.loc.gov/mods/v3 would be used for mods.  Notice the ns in the xpath above.  The namespace given here will be aliased with the prefix ns.',
      'interactive' => 'if TRUE then you will be asked to confirm the update for each object',
    ),
    'examples' => array(
      'drush -u 1 --uri=http://137.149.200.19/scholardev islandoraum /var/www/html/drupal/sites/137.149.200.19.scholardev/files/query.txt /var/www/html/drupal/sites/department_update.txt MODS "//ns:identifier[@type=\'u2\']" http://www.loc.gov/mods/v3 TRUE',
    ),
    'aliases' => array('islandoraum'),
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
function drush_islandora_update_metadata($query_file, $substitution_text_file, $datastream_id, $xpath, $namespace, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($query_file)) {
    drush_print(" using query file" . $query_file);
  }
  else {
    drush_print(" no query_file found");
    return;
  }
  if (isset($substitution_text_file)) {
    drush_print(" using text file " . $substitution_text_file);
  }
  else {
    drush_print("no text file found");
    return;
  }
  if (isset($xpath)) {
    drush_print("using xpath " . $xpath);
  }
  islandora_update_metadata_go($query_file, $substitution_text_file, $datastream_id, $xpath, $namespace, $interactive);
}

/**
 * Iterates through all the objects returned by the itql query and updates the mods stream
 * 
 * @param string $query_file
 * @param string $substitution_text
 * @param boolean $interactive
 * 
 */
function islandora_update_metadata_go($query_file, $substitution_text_file, $datastream_id, $xpath, $namespace, $interactive = FALSE) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $query = file_get_contents($query_file);
  $substitution_text = file_get_contents($substitution_text_file);
  if (!isset($query)) {
    drush_die('no query found');
    return;
  }
  if (!isset($substitution_text)) {
    drush_die('no substitution text loaded');
    return;
  }
  $results = islandora_update_run_query($query);
  $objects = islandora_update_sparql_results_as_array($results);
  foreach ($objects as $object) {
    drush_print($object);
  }
  $num = count($objects);
  if (!drush_confirm(dt('are you sure you want to update all @num objects?', array('@num' => $num)))) {
    // was: return drush_set_error('CORE_SYNC_ABORT', 'Aborting.');
    drush_die('Aborting.');
  }
  $arr = islandora_build_substitution_list($substitution_text);
  $a_keys = array_keys($arr);
  $a_values = array_values($arr);
  $count = 0;
  if ($interactive == 'TRUE') {
    foreach ($objects as $object) {
      if (drush_confirm(dt('update departments in this @pid ?', array('@pid' => $object)))) {
        //$item = new Fedora_Item($object);
        islandora_update_datastream($object, $a_keys, $a_values, $datastream_id, $xpath, $namespace);
        drush_print("processed $object # " . $count++);
      }
    }
  }
  else {
    foreach ($objects as $object) {
      //$item = new Fedora_Item($object);
      islandora_update_datastream($object, $a_keys, $a_values, $datastream_id, $xpath, $namespace);
      drush_print("processed $object # " . $count++);
    }
  }
}

/**
 * returns an array of pids 
 * @todo pull this up into an api
 * @param SimpleXMLElement $content
 * @return array
 */
function islandora_update_sparql_results_as_array($content) {
  $content = new SimpleXMLElement($content);

  $resultsarray = array();
  foreach ($content->results->result as $result) {
    $resultsarray[] = substr($result->object->attributes()->uri, 12); // Remove 'info:fedora/'.
  }
  return $resultsarray;
}

/**
 * update all the objects
 * 
 * @param string $pid
 * 
 */
function islandora_update_datastream($pid, $a_keys, $a_values, $dsid, $xpath, $namespace) {
  module_load_include('inc', 'islandora_book', 'book_pack_utils');
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'islandora_fedora_api', 'islandora_fedora_api.raw');

  $item = new Fedora_Item($pid);
  if (!isset($item)) {
    islandora_update_write_log(dt('error creating object %pid', array('%pid' => $pid)),'islandora_update.log','error');
  }

  $metadata = $item->get_datastream_dissemination($dsid);
  $sxml = simplexml_load_string($metadata);
  if (!$sxml) {
    islandora_update_write_log(dt('error processing %pid', array('%pid' => $pid)),'islandora_update.log','error');
    $item->forget();
    return;
  }
  if ($namespace != 'NULL') {//we will look for a string of NULL in case we don't want namespace
    $sxml->registerXPathNamespace('ns', $namespace);
  }
  $elements = $sxml->xpath($xpath);
  $updated = FALSE;
  foreach ($elements as $node) {
    $original_str = (string) $node;
    islandora_update_write_log(dt('processing %pid', array('%pid' => $pid)),'islandora_update.log');
    $new_str = str_replace($a_keys, $a_values, $original_str);
    islandora_update_write_log(dt('original string = %org_string new string = %new_string', array('%org_string' => $original_str, '%new_string' => $new_str)),'islandora_update.log');
    if (trim($new_str) != trim($original_str)) {
      $updated = TRUE;
      $node[0] = $new_str;
    }
  }
  if ($updated) {

    $content = $sxml->asXML();
    if (isset($content))
      $value = $item->modify_datastream($content, $dsid, $dsid, 'text/xml', $force = FALSE, $logMessage = 'Modified by Islandora API using islandora_update_metadata drush script', TRUE);
    if (isset($value)) {
      islandora_update_write_log(dt('successfully updated %dsid for %pid', array('%dsid' => $dsid, '%pid' => $pid)),'islandora_update.log');
    }
    else {
      islandora_update_write_log(dt('error updating %dsid for %pid', array('%dsid' => $dsid, '%pid' => $pid)),'islandora_update.log', 'error');
    }
    time_nanosleep(0, 20000000);
  }
  $item->forget();
}

/**
 * @todo pull this function up into an api and remove from here
 * executes a supplied itql query
 * @param string $query
 * @return type 
 */
function islandora_update_run_query($query) {
  module_load_include('inc', 'fedora_repository', 'CollectionClass');
  $cc = new CollectionClass();
  $results = $cc->getRelatedItems(NULL, $query, 1000000);
  if (isset($results)) {//still doesn't tell us if these are valid results
    return $results;
  }
  else {
    islandora_update_write_log('Error get related items, relationships cannot be updated','islandora_update.log','error');
  }
}

/**
 * writes to a log file.  
 * for now not using drush built in logging as it wants to hold everything and be
 * able to pass from one drush script ot another and we maybe processing many items at once.
 * @param type $message
 * @param string $logfile
 * @return type 
 */
function islandora_update_write_log($message, $logfile='', $status='info') {
  // Determine log file
  if($logfile == '') {
    // checking if the constant for the log file is defined
    if (defined(DEFAULT_LOG) == TRUE) {
        $logfile = DEFAULT_LOG;
    }
    // the constant is not defined and there is no log file given as input
    else {
        error_log('No log file defined!',0);
        return array(status => false, message => 'No log file defined!');
    }
  }
 
  
    $time = time(); 
 
  // Format the date and time
  $date = date("Y-m-d H:i:s", $time);
 
  // Append to the log file
  if($fd = @fopen($logfile, "a")) {
    $result = fputcsv($fd, array($status, $date, $message));
    fclose($fd);
 
    if($result > 0)
      return array('status' => true);  
    else
      return array('status' => false, message => 'Unable to write to '.$logfile.'!');
  }
  else {
    return array('status' => false, message => 'Unable to open log '.$logfile.'!');
  }
}


?>
