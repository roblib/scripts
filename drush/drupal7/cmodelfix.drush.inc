<?php

/**
 * @file
 *  Put this file where ever you put your drush commands
 *  Example: /sites/all/drush
 */

/**
 * Implementation of hook_drush_command().
 *
 * @return array
 *   An associative array describing your command(s).
 */
function cmodelfix_drush_command() {

  $items = array();
  $items['fix-them'] = array(
    'description' => "Simple Drush script which accepts a list of PIDs in a text file to update the RELS-EXT hasModel relationship.",
    'arguments' => array(
      'file' => 'The path relative to the Drupal root and file name + .txt extension of a list of PIDs to be updated.',
      'old_cmodel_pid' => 'Namespace prefix and name (PID) of the hasModel relationship to find and update.',
      'new_cmodel_pid' => 'Namespace prefix and name (PID) of the hasModel relationship to update to.',
      'interactive' => 'If true user will be asked to confirm operation on each object.',
    ),
    'examples' => array(
      'drush -u 1 cfix sites/all/drush/update_pids.txt islandora:newspaperCollectionCModel islandora:newspaperCModel true',
      'drush -u 1 cfix sites/all/drush/update_pids.txt add islandora:newspaperCModel true',
      'drush -u 1 cfix sites/all/drush/update_pids.txt islandora:newspaperCollectionCModel delete true',
    ),
    'aliases' => array('cfix'),
    // We can pass in users id on the command line using drush -u.
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );

  return $items;
}

/**
 * This is the drush command specified in the array create by the drush entry point.
 *
 * @param string
 *  The path relative to the drupal root and file name + .txt extension of a list of PIDs to be updated.
 * @param string
 *  PID of the cmodel requiring an update.
 * @param string
 *  PID of the cmodel to update to.
 * @param boolean
 *  If true user will be asked to confirm operation on each object.
 */
function drush_cmodelfix_fix_them($file, $old_cmodel_pid, $new_cmodel_pid, $interactive = FALSE) {

  if ($old_cmodel_pid == "add")
    $mode = "ADD";
  elseif ($new_cmodel_pid == "delete")
    $mode = "DELETE";

  drush_print("\n\n*************\n* CModelFix *");
  if (isset ($mode))
    drush_print("   $mode  ");

  drush_print("*************");

  // Read the given file and create array of PIDs to update
  $update_pids = file($file, FILE_IGNORE_NEW_LINES);

  if ($update_pids) {

    // Track updates found and fixed
    $GLOBALS['fixesFound'] = 0;
    $GLOBALS['fixesMade'] = 0;

    if ($interactive = strtoupper($interactive) == 'TRUE') {
      drush_print("\n>>> INTERACTIVE MODE ENGAGED");
    }

    else {
      $interactive = FALSE;
      drush_print("\n>>> AUTO MODE ENGAGED");
    }

    foreach ($update_pids as $pid) {
      $object = islandora_object_load($pid, $old_cmodel_pid);

      if ($object) {
        if ($interactive) {
          if ($mode == "DELETE") {
            if (drush_confirm(dt('@newlineSearch for relationship to delete on object @pid', array('@newline' => "\n", '@pid' => $pid)))) {
              // Search for cmodel fix and continue if match found.
              if (drush_cmodel_find_fixes($object, $old_cmodel_pid)) {
                if (drush_confirm(dt('@newlineDelete relationship @old for object @pid',
                  array('@newline' => "\n", '@pid' => $pid, '@old' => $old_cmodel_pid)))) {
                  drush_cmodel_delete ($object, $old_cmodel_pid);
                }
                else {
                  drush_print("Skipping deletion of $pid");
                }
              }
            }
            else {
              drush_print("\nSkipping search for $pid");
            }
          }
          elseif ($mode == "ADD") {

            if (drush_confirm(dt('@newlineAdd relationship @new for object @pid',
                 array('@newline' => "\n", '@pid' => $pid, '@new' => $new_cmodel_pid)))) {
              drush_cmodel_add ($object, $new_cmodel_pid);
            }
            else {
                drush_print("$new_cmodel_pid relationship NOT added to $pid");
              }
          }
          else {
            if (drush_confirm(dt('@newlineSearch for required update on object @pid', array('@newline' => "\n", '@pid' => $pid)))) {
              // Search for cmodel fix and continue if match found.
              if (drush_cmodel_find_fixes($object, $old_cmodel_pid)) {
                if (drush_confirm(dt('@newlineUpdate relationship for object @pid from@newline@old to @new',
                  array('@newline' => "\n", '@pid' => $pid, '@old' => $old_cmodel_pid, '@new' => $new_cmodel_pid)))) {
                    drush_cmodel_fix($object, $old_cmodel_pid, $new_cmodel_pid);
                }
                else {
                    drush_print("Skipping update on: $pid");
                }
              }
            }
            else {
                drush_print("\nSkipping search for fix on: $pid");
            }
          }
        }
        else {
          // Auto mode.
          if ($mode == "ADD") {
            drush_cmodel_add($object, $new_cmodel_pid);
          }
          elseif ($mode == "DELETE") {
            if (drush_cmodel_find_fixes($object, $old_cmodel_pid)) {
              drush_cmodel_delete($object, $old_cmodel_pid);
            }
          }
          else {
            if (drush_cmodel_find_fixes($object, $old_cmodel_pid)) {
              drush_cmodel_fix($object, $old_cmodel_pid, $new_cmodel_pid);
            }
          }
        }
      }

      else {
        drush_print("\nObject not found.");
      }
    }

    unset($object);
    if ($mode == "DELETE") {
      drush_print("\nCModelFix complete.\n\n***************\n    Found: " . $GLOBALS['fixesFound'] . "\n  Deleted: " . $GLOBALS['fixesMade'] . "\n***************\n");
    }
    elseif ($mode == "ADD") {
      drush_print("\nCModelFix complete.\n\n***************\n    Added: " . $GLOBALS['fixesMade'] . "\n***************\n");
    }
    else {
      drush_print("\nCModelFix complete.\n\n*************\n  Found: " . $GLOBALS['fixesFound'] . "\n  Fixed: " . $GLOBALS['fixesMade'] . "\n*************\n");
    }
  }

  else {
    drush_print("\nError reading infile \"$file\", CModelFix terminating.\n");
  }
}

function drush_cmodel_find_fixes($object, $old_cmodel_pid) {

  // @TODO verify $rels
  $rels = $object->relationships->get('info:fedora/fedora-system:def/model#', 'hasModel');

  $fix_found = FALSE;
  $no_match = 0;

  foreach ($rels as $rel) {
    if (isset($rel['object']['value'])) {
      if ($rel['object']['value'] == $old_cmodel_pid) {
        drush_print("\n>>> MATCH FOUND >>> Relationship matching \"$old_cmodel_pid\" was found on object $object->id");
        $fix_found = TRUE;
        $GLOBALS['fixesFound']++;
      }
      else {
        $no_match++;
        //$fix_found ? TRUE : FALSE;
      }
    }
    else {
      drush_print("\nNo relationships found for object $object->id.");
    }
  }

  if (!$fix_found AND $no_match > 0) {
    $message = "\nNo relationship matching \"$old_cmodel_pid\" was found on object $object->id";
  }
  elseif ($fix_found AND $no_match > 0) {
     $message = ">>> Other relationship(s) not matching \"$old_cmodel_pid\" were also found.";
  }

  //drush_print("nomatch: " . $no_match);

  if ($message) {
    drush_print($message);
  }

  return $fix_found;
}

function drush_cmodel_fix($object, $old_cmodel_pid, $new_cmodel_pid) {

  try {
      $object->relationships->remove('info:fedora/fedora-system:def/model#', 'hasModel', $old_cmodel_pid);
      $object->relationships->add('info:fedora/fedora-system:def/model#', 'hasModel', $new_cmodel_pid);
      drush_print("\n>>> UPDATE SUCCESS >>> $object->id");
      $GLOBALS['fixesMade']++;
  }
  catch (Exception $e) {
    drush_print ("Update failed for: $object->id \n\n$e->getMessage()");
  }
}

function drush_cmodel_add ($object, $new_cmodel_pid) {

  try {
    $object->relationships->add('info:fedora/fedora-system:def/model#', 'hasModel', $new_cmodel_pid);
    drush_print("\n>>> RELATIONSHIP ADDED >>> $object->id");
    $GLOBALS['fixesMade']++;
  }
  catch (Exception $e) {
    drush_print ("Addition failed for: $object->id \n\n$e->getMessage()");
  }
}

function drush_cmodel_delete ($object, $old_cmodel_pid) {

  try {
    $object->relationships->remove('info:fedora/fedora-system:def/model#', 'hasModel', $old_cmodel_pid);
    drush_print("\n>>> RELATIONSHIP DELETED >>> $object->id");
    $GLOBALS['fixesMade']++;
  }
  catch (Exception $e) {
    drush_print ("Deletion failed for: $object->id \n\n$e->getMessage()");
  }
}