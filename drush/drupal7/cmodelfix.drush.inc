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
    'description' => "accepts a list of PIDs in a text file to update the REL-EXT from using newpaperCollectionCModel to newspaperCModel",
    'arguments' => array(
      'file' => 'the path relative to the drupal root and file name + .txt extension of a list of PIDs to be updated',
      'interactive' => 'if TRUE user will be asked to confirm operation on each object',
    ),
    'examples' => array(
      'drush -u 1 cmodelfix newspapers:68 sites/all/drush/bad_pids.txt',
    ),
    'aliases' => array('cmodelfix'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // We can pass in users id on the command line using drush -u.
  );
  return $items;
}

/**
 * This is the drush command specified in the array create by the drush entry point.
 *
 * @param string
 *  the path relative to the drupal root and file name + .txt extension of a list of PIDs to be updated
 * @param boolean
 *  if TRUE user will be asked to confirm operation on each object
 */
function drush_cmodelfix_fix_them($file, $interactive = FALSE) {
  drush_print("\n\nScript running...\n");
  // Read the file and create array of pids to update
  $updatePIDS = file($file, FILE_IGNORE_NEW_LINES);
  if($updatePIDS) {
    if (strtoupper($interactive) == 'TRUE') {
      foreach ($updatePIDS as $pid) {
        $object = islandora_object_load($pid);
        if($object) {
            if (drush_confirm(dt('@newlineSearch for required update on object @pid', array('@newline' => "\n", '@pid' => $pid)))) {
              $rels = $object->relationships->get('info:fedora/fedora-system:def/model#', 'hasModel');
              //drush_print("\nCurrent relationships:\n");
              //print_r($rels);
              foreach($rels as $rel) {
                if($rel['object']['value'] == "islandora:newspaperCollectionCModel") {
                  try {
                    if (drush_confirm(dt('@newlineRelationship requiring an update was found for object @pid, continue with update', array('@newline' => "\n", '@pid' => $pid)))) {
                      $object->relationships->remove('info:fedora/fedora-system:def/model#', 'hasModel', 'islandora:newspaperCollectionCModel');
                      $object->relationships->add('info:fedora/fedora-system:def/model#', 'hasModel', 'islandora:newspaperCModel');
                      drush_print("\nSuccessfully updated: $pid");
                      //$rels = $object->relationships->get('info:fedora/fedora-system:def/model#', 'hasModel');
                      //drush_print("\nUpdated relationships:\n");
                      //print_r($rels);
                    }
                    else {
                     drush_print("Required update for $pid has been skipped.");
                    }
                  } catch (Exception $e){
                    drush_print("Update failed for: " . $pid . " " . $e->getMessage());
                  }
                }
                else {
                  drush_print("\nNo relationship needing an update was found.");
                }
              }
            }
            else {
              drush_print("Skipping: " . $pid);
            }
        }
        else {
          drush_print("Cannot find object.");
        }
      }
      unset($object);
      drush_print("\nScript terminating.\n");
    }
    else {
      drush_print("Must use interactive mode for now, script terminating.\n");
    }
  }
  else {
    drush_print("Error reading infile, script terminating.");
  }
}