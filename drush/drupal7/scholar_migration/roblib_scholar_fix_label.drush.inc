<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 islandora_gsearch_update path/to/query.txt gsearch_solr http://localhost:8080/fedoragsearch/rest TRUE
 * the above would give you an interactive update
 * if the last parameter is not TRUE then all the pids in the list will be indexed without prompting
 * 
 */

//drush hook

function roblib_scholar_fix_label_drush_command() {
  $items = array();

  $items['roblib_scholar_fix_label'] = array(
    'description' => "update the foxml label property based on the mads title and if the ownerid is empty will populate the owner id
    with either fedoraAdmin or the value of mads identifier type = u1",
    'arguments' => array(
      'pidlist_file' => 'The path to a plain text file that contains a list of pids one pid per line',
      'datastream_id' => 'the datastream id to use as the source of the label and/or owner id, only supports MADS or MODS',
      'interactive' => 'if TRUE then you will be asked to confirm the export for each object'
    ),
    'examples' => array(
      'drush -u 1 roblib_scholar_fix_label path/to/pidlist.txt dsid TRUE',
    ),
    'aliases' => array('rsfl'),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_roblib_scholar_fix_label($pidlist_file, $dsid, $interactive) {
  drush_print('Current working directory ' . getcwd());
  $pids = NULL;
  if (isset($pidlist_file)) {
    drush_print("Using PID file " . $pidlist_file . "\n");
    $pids = file($pidlist_file);
  }
  else {
    drush_print("no pidlist file specified");
    return;
  }

  roblib_scholar_fix_label_doAction($pids, $dsid, $interactive);
}

//just a function
function roblib_scholar_fix_label_doAction($pids, $dsid, $interactive) {

  foreach ($pids as $pid) {
    drush_print(trim($pid));
  }
  $num = count($pids);
  if (!drush_confirm(dt('are you sure you want to update @num objects?', array('@num' => $num)))) {
    drush_die('Aborting.');
  }

  if ($interactive == 'TRUE') {
    foreach ($pids as $pid) {
      $pid = trim($pid);
      if (drush_confirm(dt('fix label for @pid ?', array('@pid' => $pid)))) {
        roblib_scholar_fix_label_and_owner($pid, $dsid);
      }
    }
  }
  else {
    foreach ($pids as $pid) {
      $pid = trim($pid);
      roblib_scholar_fix_label_and_owner($pid, $dsid);
    }
  }
}

/**
 * Update the owner and label property if they are not already set.
 * @param string $pid
 * @return bool
 *   return FALSE if we could not read and update the MODS TRUE on success
 */
function roblib_scholar_fix_label_and_owner($pid, $dsid) {
  $object = islandora_object_load($pid);
  $namespace = strtolower($dsid);
  $namespace_string = ($dsid == 'MADS') ? 'http://www.loc.gov/mads/v2' : 'http://www.loc.gov/mods/v3';
  $datastream = $object[$dsid]->content;
  if (empty($datastream)) {
    drush_print(dt("no $dsid datastream found for object @pid, cannot update",
      array('@pid' => $pid)));
    return FALSE;
  }
  $xml_doc = new DOMDocument();
  $xml_doc->loadXML($datastream);
  if($object->label == 'Array' || $object->label == '') {
    $query = "//$namespace:titleInfo/$namespace:title";
    $xpath = new DOMXPath($xml_doc);
    $xpath->registerNamespace($namespace, $namespace_string);
    $results = $xpath->query($query, $xml_doc);
    if ($results->length > 0) {
      // we can have only one label
      $object->label = $results->item(0)->nodeValue;
      drush_print(dt('Updated the label for @pid, set the label to @label',
        array('@pid' => $pid, '@label' => $object->label)));
    }
  }

  if(empty($object->owner)) {
    //identifier type="u1"
    $query = "//$namespace:identifier[@type='u1']";
    $xpath = new DOMXPath($xml_doc);
    $xpath->registerNamespace($namespace, $namespace_string);
    $results = $xpath->query($query, $xml_doc);
    if ($results->length > 0) {
      // we can have only one object owner
      $object->owner = $results->item(0)->nodeValue;
      drush_print(dt('Updated the owner for @pid, set the owner to @owner',
        array('@pid' => $pid, '@owner' => $object->owner)));
    } else {
      $object->owner = 'fedoraAdmin';
      drush_print(dt('Updated the owner for @pid, set the owner to fedoraAdmin',
        array('@pid' => $pid)));
    }
  } else {
    drush_print(dt('Owner was already set to @owner for object @pid',
      array('@pid' => $pid, '@owner' => $object->owner)));
  }

  unset($object);

  return TRUE;
}






