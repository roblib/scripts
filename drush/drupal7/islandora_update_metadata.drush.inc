<?php

/*
 * put this file where ever you put your drush commands
 * 
 *
 * 
 */

//drush hook
define(UPDATE_LOGFILE, '/tmp/islandora_update.log');

function islandora_update_metadata_drush_command() {
  $items = array();

  $items['islandora_update_metadata'] = array(
    'description' => "scans the supplied datastream for fields with certain values and corrects known errors, this version requires drupal7 islandora with islandora_solr_search enabled",
    'arguments' => array(
      'pids_file' => 'The path to a plain text file that contains a list of pids.  One pid per line',
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
function drush_islandora_update_metadata($pids_file, $substitution_text_file, $datastream_id, $xpath, $namespace, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($pids_file)) {
    drush_print(" using file" . $pids_file);
  }
  else {
    drush_print(" no pids file found");
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
  islandora_update_metadata_go($pids_file, $substitution_text_file, $datastream_id, $xpath, $namespace, $interactive);
}

/**
 * Iterates through all the objects returned by the itql query and updates the mods stream
 * 
 * @param string $query_file
 * @param string $substitution_text
 * @param boolean $interactive
 * 
 */
function islandora_update_metadata_go($pids_file, $substitution_text_file, $datastream_id, $xpath, $namespace, $interactive = FALSE) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $object_pids = file($pids_file);
  $substitution_text = file_get_contents($substitution_text_file);
  global $user;
  
  if (!isset($substitution_text)) {
    drush_die('no substitution text loaded');
    return;
  }
  
  foreach ($object_pids as $object) {
    drush_print($object);
  }
  $num = count($object_pids);
  if (!drush_confirm(dt('are you sure you want to update all @num objects?', array('@num' => $num)))) {
    drush_die('Aborting.');
  }
  $arr = islandora_solr_build_substitution_list($substitution_text);
  $a_keys = array_keys($arr);
  $a_values = array_values($arr);
  $count = 0;
  if ($interactive == 'TRUE') {
    foreach ($object_pids as $object) {
      if (drush_confirm(dt('update @datastream_id in this @pid ?', array('@datastream_id' => $datastream_id, '@pid' => $object)))) {
        //$item = new Fedora_Item($object);
        islandora_update_metadata_update_datastream($object, $a_keys, $a_values, $datastream_id, $xpath, $namespace);
        drush_print("processed $object # " . $count++);
      }
    }
  }
  else {
    foreach ($object_pids as $object) {
      //$item = new Fedora_Item($object);
      islandora_update_metadata_update_datastream($object, $a_keys, $a_values, $datastream_id, $xpath, $namespace);
      drush_print("processed $object # " . $count++);
    }
  }
}


/**
 * update all the objects
 * 
 * @param string $pid
 * 
 */
function islandora_update_metadata_update_datastream($pid, $a_keys, $a_values, $dsid, $xpath, $namespace) {
  $pid = trim($pid);
  $item = islandora_object_load($pid);
  if (!$item) {
    islandora_update_metadata_write_log(dt('error loading object @pid', array('@pid' => $pid)),UPDATE_LOGFILE,'error');
    return;
  }
  
  $metadata = $item[$dsid]->content;
  $sxml = simplexml_load_string(trim($metadata));
  
  if ($sxml === FALSE) {
    islandora_update_metadata_write_log(dt('error processing @pid', array('@pid' => $pid)),UPDATE_LOGFILE,'error');
    unset($item);
    return;
  }
  if ($namespace != 'NULL') {//we will look for a string of NULL in case we don't want namespace
    $sxml->registerXPathNamespace('ns', $namespace);
  }
  $elements = $sxml->xpath($xpath);
  $updated = FALSE;
  foreach ($elements as $node) {
    $original_str = (string) $node;
    islandora_update_metadata_write_log(dt('processing @pid', array('@pid' => $pid)),UPDATE_LOGFILE);
    $new_str = str_replace($a_keys, $a_values, $original_str);
    islandora_update_metadata_write_log(dt('original string = @org_string, new string = @new_string', array('@org_string' => $original_str, '@new_string' => $new_str)),UPDATE_LOGFILE);
    if (trim($new_str) != trim($original_str)) {
      $updated = TRUE;
      $node[0] = $new_str;
    }
  }
  if ($updated) {

    $content = $sxml->asXML();
    if (isset($content))
      $value = $item[$dsid]->content = $content;
    if (isset($value)) {
      islandora_update_metadata_write_log(dt('successfully updated @dsid for @pid', array('@dsid' => $dsid, '@pid' => $pid)),UPDATE_LOGFILE);
    }
    else {
      islandora_update_metadata_write_log(dt('error updating @dsid for @pid', array('@dsid' => $dsid, '@pid' => $pid)),UPDATE_LOGFILE, 'error');
    }
    time_nanosleep(0, 20000000);
  }
  unset($item);
}


/**
 * writes to a log file.  
 * for now not using drush built in logging as it wants to hold everything and be
 * able to pass from one drush script ot another and we maybe processing many items at once.
 * @param type $message
 * @param string $logfile
 * @return type 
 */
function islandora_update_metadata_write_log($message, $logfile='', $status='info') {
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
