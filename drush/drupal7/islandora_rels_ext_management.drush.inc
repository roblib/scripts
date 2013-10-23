<?php

/**
 * @file
 * Put this file where ever you put your drush commands.
 *
 *  Example: /sites/all/drush
 */

/**
 * Implementation of hook_drush_command().
 *
 * @return array
 *   An associative array describing your command(s).
 */
function islandora_rels_ext_management_drush_command() {

  $items = array();
  $items['change_collection_relationship'] = array(
    'description' => "Change the collection relationship, for example change isMemberOfCollection to isMemberOf.",
    'arguments' => array(
      'file' => 'The path relative to the Drupal root and file name + .txt extension of a list of PIDs to be updated.',
      'old_predicate' => 'The old relationship predicate (for example isMemberOfCollection)',
      'new_predicate' => 'The new relationship predicate (for example isMemberOf)',
      'interactive' => 'If TRUE user will be asked to confirm operation on each object.',
    ),
    'examples' => array(
      'drush -u 1 islandora_ccr sites/all/drush/pids.txt isMemberOfCollection isMemberOf TRUE',
    ),
    'aliases' => array('islandora_ccr'),
    // We can pass in users id on the command line using drush -u.
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );

  return $items;
}

/**
 * Drush entry point.
 *
 * @param string $file
 *   A path to a file that contains a list of pids one per line
 * @param string $old_predicate
 *   The relationhsip we want to change ie. isMemberOfCollection.
 * @param string $new_predicate
 *   The relationship we want to change to ie isMemberOf.
 * @param bool $interactive
 *   If interactive is TRUE the script will prompt to update each object
 */
function drush_islandora_rels_ext_management_change_collection_relationship($file, $old_predicate, $new_predicate, $interactive = FALSE) {
  if (isset($file)) {
    drush_print(dt(" using file @file", array('file' => $file)));
  }
  else {
    drush_print(dt(" no file found"));
    return;
  }
  ini_set("auto_detect_line_endings", "1");
  $pids = file($file);
  if (!empty($pids)) {
    $num = count($pids);
    if (!drush_confirm(dt('are you sure you want to update all @num objects?', array('@num' => $num)))) {
      drush_print(dt('Aborting.'));
      return;
    }
    islandora_rels_collection_relationship_start($pids, $old_predicate, $new_predicate, $interactive);
  }
  else {
    drush_print(dt('File @file is empty or does not exist.', array('@file' => $file)));
  }
}

/**
 * Start processing the list of pids.
 *
 * @param array $pids
 *   The array pids of the objects we want to update.
 * @param string $old_predicate
 *   The relationhsip we want to change ie. isMemberOfCollection.
 * @param string $new_predicate
 *   The relationship we want to change to ie isMemberOf.
 * @param bool $interactive
 *   If interactive is TRUE the script will prompt to update each object
 */
function islandora_rels_collection_relationship_start($pids, $old_predicate, $new_predicate, $interactive) {
  $count = 0;
  $updated_count = 0;
  $not_updated_count = 0;
  $updated = FALSE;
  foreach ($pids as $pid) {
    $count++;
    $pid = trim($pid);
    if (!empty($pid)) {
      if (strtoupper($interactive) == 'TRUE') {
        if (drush_confirm(dt('update this @pid ?', array('@pid' => $pid)))) {
          $updated = islandora_process_rels_collection_relationship($pid, $old_predicate, $new_predicate);
        }
      }
      else {
        $updated = islandora_process_rels_collection_relationship($pid, $old_predicate, $new_predicate);
      }
      if ($updated) {
        $updated_count++;
      }
      else {
        $not_updated_count++;
      }
      if ($count > 100) {
        time_nanosleep(0, 500000000);
        $count = 0;
      }
    }
  }
  drush_print(dt('updated @count objects', array('@count' => $updated_count)));
  drush_print(dt('failed to update @count objects', array('@count' => $not_updated_count)));
}


/**
 * Update the relationship an object.
 *
 * If this object already has the specified relationship we don't touch it.
 * TODO: modify this so it handles objects with multiple relationships better.
 * Currently if the object has multiple relationships and any one of those
 * includes the new_predicate we will fail to update the object even if it has
 * the old predicate.
 *
 * @param string $pid
 *   The pid of the object that we want to modify.
 * @param string $old_predicate
 *   The relationhsip we want to change ie. isMemberOfCollection.
 * @param string $new_predicate
 *   The relationship we want to change to ie isMemberOf.
 *
 * @return bool
 *   returns TRUE if it actually modifies an object FALSE otherwise
 */
function islandora_process_rels_collection_relationship($pid, $old_predicate, $new_predicate) {
  $object = islandora_object_load($pid);
  if (empty($object)) {
    drush_print(dt("no object found for pid @pid", array('@pid' => $pid)));
    return FALSE;
  }
  $new_rels = $object->relationships->get('info:fedora/fedora-system:def/relations-external#', $new_predicate);
  if ($new_rels) {
    drush_print(dt("Object @pid already has a relationship with @new_predicate not updating this object", array(
      '@pid' => $pid,
      '@new_predicate' => $new_predicate,
    )));
    return FALSE;
  }
  $old_rels = $object->relationships->get('info:fedora/fedora-system:def/relations-external#', $old_predicate);
  $updated = 'NO';
  if ($old_rels) {
    foreach ($old_rels as $rel) {
      $related_to = $rel['object']['value'];
      $object->relationships->remove('info:fedora/fedora-system:def/relations-external#', $old_predicate, $related_to);
      $object->relationships->add('info:fedora/fedora-system:def/relations-external#', $new_predicate, $related_to);
      $updated = 'YES';
    }
  }
  unset($object);
  drush_print(dt("processed rels of @pid, modified = @updated", array(
    '@pid' => $pid,
    '@updated' => $updated,
  )));
  return $updated == 'YES' ? TRUE : FALSE;
}
