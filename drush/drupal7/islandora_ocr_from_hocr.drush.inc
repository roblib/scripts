<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 
 * 
 */

//drush hook

function islandora_ocr_from_hocr_drush_command() {
  $items = array();

  $items['islandora_ocr_from_hocr'] = array(
    'description' => "Reads a csv file first column is pid.  
      Currently requires Drupal7 Islandora with Tuque.  
      Each pid in the txt/csv file should represent an object with an hocr, the
      script will then strip the tags from the hocr and save it as a plain ocr
      stream. This is faster then rerunning tesseract (especially on newspaper
      type pages).",
    'arguments' => array(
      'csv_file' => 'The path to a csv file that contains columns as described in the description.',
      'interactive' => 'if TRUE then you will be asked to confirm the update for each object',
    ),
    'examples' => array(
      'drush -u 1 --uri=http://137.149.200.19/scholardev islandora_ocr_f_hocr /var/www/html/drupal/sites/137.149.200.19.scholardev/files/csv.txt TRUE',
    ),
    'aliases' => array('islandora_ocr_f_hocr'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

/**
 * This is the drush command specified in the array create by the drush entry point.
 * 
 * This function checks to make sure parameters are supplied and if everything is ok
 * calls our go function
 * 
 * @param string $query_file
 *  path to a text file that contains an itql query
 * @param boolean $interactive
 * 
 */
function drush_islandora_ocr_from_hocr($csv_file, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($csv_file)) {
    drush_print(" using csv file" . $csv_file);
  }
  else {
    drush_print(" no csv file found");
    return;
  }

  islandora_ocr_from_hocr_go($csv_file, $interactive);
}

/**
 * Iterates through all the objects returned by the itql query and updates the mods stream
 * 
 * @param string $csv_file
 * @param boolean $interactive
 * @param string $action
 * 
 */
function islandora_ocr_from_hocr_go($csv_file, $interactive = FALSE) {

  ini_set("auto_detect_line_endings", "1");

  $line_number = 0;
  $lines = file($csv_file);
  $count = 0;
  foreach ($lines as $pid) {
    $count++;
    $pid = trim($pid);
    if (!empty($pid)) {

      if ($interactive == 'TRUE') {
        if (drush_confirm(dt('update this @pid ?', array('@pid' => $pid)))) {
          //$item = new Fedora_Item($object);
          islandora_drush_ocr_from_hocr($pid);
          drush_print("processed hocr of $pid # " . ++$line_number);
        }
      }
      else {
        islandora_drush_ocr_from_hocr($pid);
        drush_print("processed hocr of $pid # " . ++$line_number);
      }

      if ($count > 100) {
        time_nanosleep(0, 500000000);
        $count = 0;
      }
    }
  }
}

function islandora_drush_ocr_from_hocr($pid) {
  $object = islandora_object_load($pid);
  if (empty($pid)) {
    drush_print("no object found for pid $pid");
    return;
  }
  $hocr = $object['HOCR']->content;
  $hocr_xml = new DOMDocument();
  $isLoaded = $hocr_xml->loadXML($hocr);
  if($isLoaded) {
    drush_print("loaded hocr for $pid");
    $raw_ocr = strip_tags($hocr_xml->saveHTML());
    $datastream = new NewFedoraDatastream('OCR', "M", $object, $object->repository);
    $datastream->state = 'A';
    $datastream->versionable = TRUE;
    $datastream->checksum = TRUE;
    $datastream->checksumType = 'MD5';

    $datastream->label = "Scanned text";
    $datastream->mimetype = "text/plain";
    $datastream->setContentFromString($raw_ocr);
    $datastream->logMessage = "ocr created by stripping tags from hocr";
    $object->ingestDatastream($datastream);
    drush_print("added OCR datastream to $pid");

  } else {
    drush_print("failed loading HOCR for $pid");
  }

  unset($object);
}

?>