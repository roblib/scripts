<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 islandora_gsearch_update path/to/query.txt gsearch_solr http://localhost:8080/fedoragsearch/rest TRUE
 * the above would give you an interactive update
 * if the last parameter is not TRUE then all the pids in the list will be indexed without prompting
 * 
 */

//drush hook

function roblib_scholar_update_dc_drush_command() {
  $items = array();

  $items['roblib_scholar_update_dc'] = array(
    'description' => "Runs the to_dc transform for this object.",
    'arguments' => array(
      'pidlist_file' => 'The path to a plain text file that contains a list of pids one pid per line',
      'cmodel' => 'The cmodel to use to get the transform',
      'dsid' => 'The dsid you want transformed to DC',
      'interactive' => 'if TRUE then you will be asked to confirm the export for each object'
    ),
    'examples' => array(
      'drush -u 1 roblib_scholar_update_dc path/to/pidlist.txt cmodel dsid TRUE',
    ),
    'aliases' => array('rsud'),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_roblib_scholar_update_dc($pidlist_file, $cmodel, $dsid, $interactive) {
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

  roblib_scholar_update_dc_doAction($pids, $cmodel, $dsid, $interactive);
}

//just a function
function roblib_scholar_update_dc_doAction($pids, $cmodel, $dsid, $interactive) {

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
      if (drush_confirm(dt('Update DC for @pid ?', array('@pid' => $pid)))) {
        roblib_scholar_update_dc($pid, $cmodel, $dsid);
      }
    }
  }
  else {
    foreach ($pids as $pid) {
      $pid = trim($pid);
      roblib_scholar_update_dc($pid, $cmodel, $dsid);
    }
  }
}

/**
 * Update the citation and thesis cmodel namespaces.
 * @param string $pid
 * @return int
 *   return FALSE if we could not read and update the MODS TRUE on success
 */
function roblib_scholar_update_dc($pid, $cmodel, $dsid) {
  $object = islandora_object_load($pid);
  if (empty($object)) {
    drush_print(dt('Could not load object @pid, cannot update',
      array('@pid' => $pid)));
    unset($object);
    return FALSE;
  }
  $cmodels = $object->models;
  if (!in_array($cmodel, $cmodels)) {
    drush_print(dt('Object @pid did not have the correct cmodel, not updating',
      array('@pid' => $pid)));
    unset($object);
    return FALSE;
  }
  if (empty($object[$dsid])) {
    drush_print(dt('Object @pid did not have the correct datastream, not updating',
      array('@pid' => $pid)));
    unset($object);
    return FALSE;
  }

  module_load_include('inc', 'xml_form_builder', 'includes/associations');
  $associations = xml_form_builder_get_associations(NULL, array($cmodel), array($dsid), TRUE);
  // if there are multiple associations we are going to use the first
  $association = reset($associations);

  $transform_path = drupal_realpath(xml_form_builder_get_transform_file($association['transform']));

  $xml = new DOMDocument();
  $test = $xml->loadXML($object[$dsid]->content);
  if (!$test) {
    drush_print(dt('Could not load the specified @dsid for Object @pid, not updating',
      array('@pid' => $pid, '@dsid' => $dsid)));
    unset($object);
    return FALSE;
  }
  $new_dc_doc = xml_form_builder_transform_document($transform_path, $xml);
  $new_dc_xml = $new_dc_doc->saveXML($new_dc_doc);
  $object['DC']->content = $new_dc_xml;

  drush_print(dt('Successfully updated DC for Object @pid based on @dsid',
    array('@pid' => $pid, '@dsid' => $dsid)));

  unset($object);

  return TRUE;
}





