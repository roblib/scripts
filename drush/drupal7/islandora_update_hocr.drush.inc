<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 
 * 
 */

//drush hook

function islandora_update_hocr_drush_command() {
  $items = array();

  $items['islandora_update_hocr'] = array(
    'description' => "Reads a csv file first column is pid.  
      Currently requires Drupal7 Islandora with Tuque.  
      Each pid in the txt/csv file should represent an object with an hocr stream that was created with tesseract with a version before 3.02.02
      This is pretty specific to UPEI's newspapers right now)",
    'arguments' => array(
      'csv_file' => 'The path to a csv file that contains columns as described in the description.',
      'xsl_file' => 'The path to an xslt to crosswalk the old hocr to hocr 3.02.02',
      'interactive' => 'if TRUE then you will be asked to confirm the update for each object',
    ),
    'examples' => array(
      'drush -u 1 --uri=http://137.149.200.19/scholardev islandorauphocr /var/www/html/drupal/sites/137.149.200.19.scholardev/files/csv.txt /path/to/xslt.xsl TRUE',
    ),
    'aliases' => array('islandorauphocr'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

/**
 * This is the drush command specified in the array create by the drush entry point.
 * 
 * This function checks to make sure parameters are supplied and if everything is ok
 * calls the doAction function
 * 
 * @param string $query_file
 *  path to a text file that contains an itql query
 * @param boolean $interactive
 * 
 */
function drush_islandora_update_hocr($csv_file, $xsl_file, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($csv_file)) {
    drush_print(" using csv file" . $csv_file);
  }
  else {
    drush_print(" no csv file found");
    return;
  }
  if (isset($xsl_file)) {
    drush_print(" using xsl file" . $xsl_file);
  }
  else {
    drush_print(" no xsl file found");
    return;
  }
  islandora_update_hocr_go($csv_file, $xsl_file, $interactive);
}

/**
 * Iterates through all the objects returned by the itql query and updates the mods stream
 * 
 * @param string $csv_file
 * @param boolean $interactive
 * @param string $action
 * 
 */
function islandora_update_hocr_go($csv_file, $xsl_file, $interactive = FALSE) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
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
          islandora_drush_update_hocr($pid, $xsl_file);
          drush_print("processed hocr of $pid # " . ++$line_number);
        }
      }
      else {
        islandora_drush_update_hocr($pid, $xsl_file);
        drush_print("processed hocr of $pid # " . ++$line_number);
      }

      if ($count > 100) {
        time_nanosleep(0, 500000000);
        $count = 0;
      }
    }
  }
}

function islandora_drush_update_hocr($pid, $xsl_file) {
  $object = islandora_object_load($pid);
  if (empty($pid)) {
    drush_print("no object found for pid $pid");
    return;
  }
  $hocr = $object['HOCR']->content;
  $hocr = str_replace('<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd" >', '', $hocr);
  if (!empty($hocr)) {
    //transform
    $xsl = file_get_contents($xsl_file);
    $xslt = new XSLTProcessor();
    $xslt->importStylesheet(new SimpleXMLElement($xsl));
    $hocr_xml = simplexml_load_string($hocr);
    $hocr_xml->registerXPathNamespace('ns', 'http://www.w3.org/1999/xhtml');
    $content_attributes = $hocr_xml->xpath('/ns:html/ns:head/ns:meta[@name="ocr-system"]/@content');
    $version = (string) reset($content_attributes);
    $version = str_replace(array('tesseract', ' '), '', $version);
    if (!empty($version) && $version == '3.02.02') {
      drush_print("Version = $version, no need to transform");
      unset($object);
      return;
    }
    $new_hocr = $xslt->transformToXml($hocr_xml);
    if (!empty($new_hocr)) {
      $object['HOCR']->content = $new_hocr;
    }
    else {
      drush_print("Error transforming HOCR for $pid");
    }
  }
  unset($object);
}

?>