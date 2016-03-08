<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 islandora_gsearch_update path/to/query.txt gsearch_solr http://localhost:8080/fedoragsearch/rest TRUE
 * the above would give you an interactive update
 * if the last parameter is not TRUE then all the pids in the list will be indexed without prompting
 * 
 */

//drush hook

function roblib_scholar_fix_cmodels_drush_command() {
  $items = array();

  $items['roblib_scholar_fix_cmodels'] = array(
    'description' => "Change the cmodel namespaces from islandora to ir.",
    'arguments' => array(
      'pidlist_file' => 'The path to a plain text file that contains a list of pids one pid per line',
      'old_cmodel' => 'The cmodel you don\'t want anymore',
      'new_cmodel' => 'The cmodel you want to change to',
      'interactive' => 'if TRUE then you will be asked to confirm the change for each object'
    ),
    'examples' => array(
      'drush -u 1 roblib_scholar_fix_cmodels path/to/pidlist.txt old_cmodel new_cmodel TRUE',
    ),
    'aliases' => array('rsfc'),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_roblib_scholar_fix_cmodels($pidlist_file, $old_cmodel, $new_cmodel, $interactive) {
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

  roblib_scholar_fix_cmodels_doAction($pids, $old_cmodel, $new_cmodel, $interactive);
}

//just a function
function roblib_scholar_fix_cmodels_doAction($pids, $old_cmodel, $new_cmodel, $interactive) {

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
      if (drush_confirm(dt('correct cmodel namespace for @pid ?', array('@pid' => $pid)))) {
        roblib_scholar_fix_cmodel($pid, $old_cmodel, $new_cmodel);
      }
    }
  }
  else {
    foreach ($pids as $pid) {
      $pid = trim($pid);
      roblib_scholar_fix_cmodel($pid, $old_cmodel, $new_cmodel);
    }
  }
}

/**
 * Update the citation and thesis cmodel namespaces.
 * @param string $pid
 * @return int
 *   return -1 if we could not read the MODS 0 on success
 */
function roblib_scholar_fix_cmodel($pid, $old_cmodel, $new_cmodel) {
  $object = islandora_object_load($pid);
  if(empty($object)){
    drush_print(dt("Could not load object for @pid, cannot update", array(
      '@pid' => $pid,
    )));
    return false;
  }
  $cmodels = $object->relationships->get('info:fedora/fedora-system:def/model#', 'hasModel', $new_cmodel);
  // check to make sure new cmodel doesn't already exist
  if (!empty($cmodels)) {
    drush_print(dt("Object @pid already has a cmodel with @new_cmodel no update required", array(
      '@pid' => $pid,
      '@new_cmodel' => $new_cmodel,
    )));
    unset($object);
    return FALSE;
  }

  $success = $object->relationships->remove(FEDORA_MODEL_URI, 'hasModel', $old_cmodel);
  if ($success) {
    drush_print(dt("removed cmodel @old_cmodel for @pid", array(
      '@pid' => $pid,
      '@old_cmodel' => $old_cmodel,
    )));
  }
  else {
    drush_print(dt("Did not remove cmodel @old_cmodel for @pid, not updating this object", array(
      '@pid' => $pid,
      '@old_cmodel' => $old_cmodel,
    )));
    // This object does not have the cmodel we want to change so exiting
    unset($object);
    return FALSE;
  }
  // Add the new hasModel statement
  $object->relationships->add(FEDORA_MODEL_URI, 'hasModel', $new_cmodel);

  drush_print(dt("added new  cmodel @new_cmodel for @pid", array(
    '@pid' => $pid,
    '@new_cmodel' => $new_cmodel,
  )));

  unset($object);
  return $success;

}




