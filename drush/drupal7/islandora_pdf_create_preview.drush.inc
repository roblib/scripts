<?php

/*
 * This file should be put in the drush/commands/islandora directory of your drush install
 * (you may have to create the islandora directory
 * 
 * updates xacml policies on all excepts specified by an itql query. 
 * 
 * @todo: add a datastream parameter then we could pass a file and update either collection_policy
 * xacml policy or any other dsid that is xml and needs to be updated across a group of objects.
 */

//drush hook

function islandora_pdf_create_preview_drush_command() {
  $items = array();

  $items['islandora_pdf_create_preview'] = array(
    'description' => "Creates the PREVIEW datastream for objects of type sp_pdf",
    'arguments' => array(
      'pid_list' => 'The path to a plain text file that contains a list of pids',
      'interactive' => 'if TRUE then you will be asked to confirm the export for each object'
    ),
    'examples' => array(
      'drush -u 1 islandora_pdf_create_preview /path/to/pidlist.txt TRUE',
    ),
    'aliases' => array('islandorapcp'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_islandora_pdf_create_preview($pid_list, $interactive) {
  drush_print('Current working directory ' . getcwd());
  $export_dir = getcwd();

  if (isset($pid_list)) {
    drush_print(" using pid list file" . $pid_list);
  }
  else {
    drush_print(" no pid list found");
    return;
  }
  islandora_pdf_drush_create_preview_go($pid_list, $interactive);
}

/**
 * Creates a PREVIEW datastream.
 *
 * @param string $pid_list
 *   A file containing a list of pids.
 * @param string $interactive
 *   If True the user will be prompted to update each object.
 *
 */
function islandora_pdf_drush_create_preview_go($pid_list, $interactive) {
  ini_set("auto_detect_line_endings", "1");
  $test = module_load_include('inc', 'islandora_pdf', 'includes/derivatives');
  $line_number = 0;
  $lines = file($pid_list);
  $count = 0;
  foreach ($lines as $pid) {
    $count++;
    $pid = trim($pid);
    if (!empty($pid)) {
      $object = islandora_object_load($pid);
      if ($interactive == 'TRUE') {
        if (drush_confirm(dt('update this @pid ?', array('@pid' => $pid)))) {
          islandora_pdf_drush_add_preview_derivative($object);
          drush_print("processed $pid # " . ++$line_number);
        }
      }
      else {
        islandora_pdf_drush_add_preview_derivative($object);
        drush_print("processed $pid # " . ++$line_number);
      }
      if ($count > 100) {
        time_nanosleep(0, 500000000);
        $count = 0;
      }
      unset($object);
    }
  }
}

function islandora_pdf_drush_add_preview_derivative($object) {
  $width = variable_get('islandora_pdf_preview_width', 500);
  $height = variable_get('islandora_pdf_preview_height', 700);
  $file_uri = islandora_pdf_get_derivative_source_as_unmanaged_file($object);
  islandora_pdf_drush_add_jpg_derivative($object, $file_uri, 'PREVIEW', $width, $height, $force);
  file_unmanaged_delete($file_uri);
}

function islandora_pdf_drush_add_jpg_derivative($object, $file_uri, $dsid, $width, $height) {
  if (isset($object[$dsid])) {
    drush_print(dt('ERROR @dsid already exists', array(
      '@dsid' => $dsid,
    )));
    return;
  }
  if (!isset($object['OBJ'])) {
    drush_print(dt('ERROR Source DSID does not exist for @id', array('@id' => $object['label'])));
    return;
  }
  $derivative_file_uri = islandora_pdf_drush_create_jpg_derivative($file_uri, $dsid, $width, $height);
  file_unmanaged_delete($file_uri);
  if($derivative_file_uri == -1){
    drush_print('error creating derivative');
    return;
  }
  // Receive a valid file URI to add or an error message otherwise.
  $success = islandora_pdf_add_datastream($object, $dsid, $derivative_file_uri);
  file_unmanaged_delete($derivative_file_uri);
  if ($success === TRUE) {
    drush_print(dt('added @dsid', array(
      '@dsid' => $dsid,
    )));
  }
  else {
    drush_print(dt('ERROR adding @dsid ', array(
      '@dsid' => $dsid,
    )));
  }
}

function islandora_pdf_drush_create_jpg_derivative($file_uri, $dsid, $width, $height) {
  $source = drupal_realpath($file_uri) . '[0]';
  //$matches = array();
  // Get the base name of the source file.
  $base = pathinfo($source, PATHINFO_FILENAME);
  $temp = file_create_filename("$base.$dsid.jpg", 'temporary://');
  $dest = drupal_realpath($temp);
  $args['quality'] = '-quality ' . escapeshellarg(variable_get('imagemagick_quality', 75));
  $args['previewsize'] = '-resize ' . escapeshellarg("{$width}x{$height}");
  $args['colors'] = '-colorspace RGB';
  $args['flatten'] = '-flatten';

  $command = 'convert ' . escapeshellarg($source) . ' ' . implode(' ', $args) . ' ' . escapeshellarg("jpg:$dest");
  $output = '';
  $ret = -1;
  exec($command, $output, $ret);
  if($ret === 0){
    return $dest;
  } else {
    return -1;
  }
}
