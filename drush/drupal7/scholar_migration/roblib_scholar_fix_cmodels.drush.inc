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
     'interactive' => 'if TRUE then you will be asked to confirm the export for each object'
    ),
    'examples' => array(
      'drush -u 1 roblib_scholar_fix_cmodels path/to/pidlist.txt TRUE',
    ),
    'aliases' => array('rsfc'),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_roblib_scholar_fix_cmodels($pidlist_file, $interactive) {
  drush_print('Current working directory ' . getcwd());
  $pids = NULL;
  if (isset($pidlist_file)) {
    drush_print("Using PID file " . $pidlist_file . "\n");
    $pids = file($pidlist_file);
  }
  else {
    drush_print("no query file specified");
    return;
  }

  roblib_scholar_fix_cmodels_doAction($pids, $interactive);
}

//just a function
function roblib_scholar_fix_cmodels_doAction($pids, $interactive) {

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
        roblib_scholar_fix_cmodel($pid);
      }
    }
  }
  else {
    foreach ($pids as $pid) {
      $pid = trim($pid);
      roblib_scholar_fix_cmodel($pid);
    }
  }
}

/**
 * Update the citation and thesis cmodel namespaces.
 * @param string $pid
 * @return int
 *   return -1 if we could not read the MODS 0 on success
 */
function roblib_scholar_fix_cmodel($pid) {
  $object = islandora_object_load($pid);
  $cmodels = $object->relationships->get('info:fedora/fedora-system:def/model#', 'hasModel');
  $new_cmodel = NULL;
  $old_cmodel = NULL;
  foreach ($cmodels as $cmodel) {
    switch ($cmodel['object']['value']) {
      case 'islandora:citationCModel':
        drush_print(dt('found @cmodel for object @pid',
          array('@cmodel' => $cmodel['object']['value'], '@pid' => $pid )));
        $new_cmodel = 'ir:citationCModel';
        $old_cmodel = $cmodel['object']['value'];
        break;
      case 'islandora:thesisCModel':
        drush_print(dt('found @cmodel for object @pid',
          array('@cmodel' => $cmodel['object']['value'], '@pid' => $pid )));
        $new_cmodel = 'ir:thesisCModel';
        $old_cmodel = $cmodel['object']['value'];
        break;
    }
  }
  // No old cmodel found this does not need to be updated
  if(empty($new_cmodel)){
    drush_print(dt('no old cmodel found for for object @pid, no update required',
      array('@pid' => $pid )));
    return FALSE;
  }
  // check to make sure new cmodel doesn't already exist
  foreach($cmodels as $cmodel){
    if($cmodel['object']['value'] == $new_cmodel){
      drush_print(dt("Object @pid already has a cmodel with @new_cmodel no update required", array(
        '@pid' => $pid,
        '@new_cmodel' => $new_cmodel,
      )));
      return FALSE;
    }
  }
  // Remove the old hasModel statement
  $object->relationships->remove(FEDORA_MODEL_URI, 'hasModel', $old_cmodel);
  // Add the new hasModel statement
  $object->relationships->add(FEDORA_MODEL_URI,'hasModel', $new_cmodel);

  unset($object);
  drush_print(dt("modified rels of @pid added @new_cmodel, removed @old_cmodel", array(
    '@pid' => $pid,
    '@new_cmodel' => $new_cmodel,
    '@old_cmodel' => $old_cmodel,
  )));
  return TRUE;

}




