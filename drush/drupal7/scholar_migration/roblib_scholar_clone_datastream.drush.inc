<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 islandora_gsearch_update path/to/query.txt gsearch_solr http://localhost:8080/fedoragsearch/rest TRUE
 * the above would give you an interactive update
 * if the last parameter is not TRUE then all the pids in the list will be indexed without prompting
 * 
 */

//drush hook

function roblib_scholar_clone_datastream_drush_command() {
  $items = array();

  $items['roblib_scholar_clone_datastream'] = array(
    'description' => "update the mods so it conforms to new islandscholar expectations (and fix some common errors in our mods).",
    'arguments' => array(
      'pidlist_file' => 'The path to a plain text file that contains a list of pids one pid per line',
      'old_dsid' => 'The datastream id of the datastream you would like to clone',
      'new_dsid' => 'The datastream id for the new datastream',
      'delete_old' => 'TRUE if you want to delete the old datastream FALSE if you want to keep it.',
      'interactive' => 'if TRUE then you will be asked to confirm the export for each object'
    ),
    'examples' => array(
      'drush -u 1 roblib_scholar_clone_datastream path/to/pidlist.txt old_dsid new_dsid FALSE TRUE',
    ),
    'aliases' => array('rscd'),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_roblib_scholar_clone_datastream($pidlist_file, $old_dsid, $new_dsid, $delete_old, $interactive) {
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

  roblib_scholar_clone_datastream_doAction($pids, $old_dsid, $new_dsid, $delete_old, $interactive);
}

//just a function
function roblib_scholar_clone_datastream_doAction($pids, $old_dsid, $new_dsid, $delete_old, $interactive) {

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
      if (drush_confirm(dt('are you sure you want to copy @old_ds to @new_ds for @pid ?',
        array('@pid' => $pid,
          '@old_ds' => $old_dsid,
          '@new_ds' => $new_dsid,
          )))) {
        roblib_scholar_clone_datastream($pid, $old_dsid, $new_dsid, $delete_old);
      }
    }
  }
  else {
    foreach ($pids as $pid) {
      $pid = trim($pid);
      roblib_scholar_clone_datastream($pid, $old_dsid, $new_dsid, $delete_old);
    }
  }
}

/**
 * Creates a new datastream based on the old datastream.
 * For instance if you need to change a datastream id.
 * @param string $pid
 * @param string $old_dsid
 * @param string $new_dsid
 * @param string $delete_old
 * @return bool
 *   Return True if the new datastream is created otherwise return false
 */
function roblib_scholar_clone_datastream($pid, $old_dsid, $new_dsid, $delete_old) {
  $object = islandora_object_load($pid);
  $old_datastream = $object[$old_dsid];
  if (empty($old_datastream)) {
    drush_print(dt('no @old_ds datastream found for for object @pid, cannot clone datastream',
      array('@pid' => $pid, '@old_ds' => $old_dsid)));
    return FALSE;
  }
  $removed_ds = ($delete_old === 'TRUE') ? 'purged' : 'did not purge';
  $datastream = new NewFedoraDatastream($new_dsid, 'M', $object, $object->repository);
  $datastream->state = $old_datastream->state;
  $datastream->versionable = $old_datastream->versionable;
  $datastream->label = 'PDF';
  $datastream->mimetype = $old_datastream->mimetype;
  $datastream->logMessage = "copied $old_dsid to $new_dsid and $removed_ds $old_dsid as part of migration to IslandScholar 7.x";
  $tmp_file = drupal_tempnam(file_directory_temp(), 'rlscmi');
  $success = $old_datastream->getContent($tmp_file);
  if($success){
    $datastream->setContentFromFile($tmp_file);
  }  else {
    drush_print(dt('Error retrieving content of @old_ds from @pid',
      array('@old_ds' => $old_dsid, '@pid' => $pid)));
    return FALSE;
  }
  $object->ingestDatastream($datastream);
  unset($object);
  drush_print(dt("copied @old_ds to @new_ds for @pid, @removed_ds @old_ds", array(
    '@pid' => $pid,
    '@old_ds' => $old_dsid,
    '@new_ds' => $new_dsid,
    '@removed_ds' => $removed_ds,
  )));
  unlink($tmp_file);
  return TRUE;
}