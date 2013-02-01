<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 
 *
 * 
 */

//drush hook

function islandora_update_metadata_from_xsl_drush_command() {
  $items = array();

  $items['islandora_update_metadata_from_xsl'] = array(
    'description' => "Reads a list of pids from a text file and updates the MODS of that object using an xslt that crosswalks refworks xml to MODS.  
      Requires Drupal 6 version of Islandora",
    'arguments' => array(
      'list_file' => 'The path to a plain text file that contains alist of pids (one per line).',
      'xslt_file' => 'The path to an xslt that can transform refworks xml to MODS.  We may have different xsls depending on genre.',
      'datastream_id' => 'the id of the datastream to MODIFY (will usually be MODS)',
      'interactive' => 'if TRUE then you will be asked to confirm the update for each object',
    ),
    'examples' => array(
      'drush -u 1 --uri=http://137.149.200.19/scholardev islandoraumxsl /var/www/html/drupal/sites/137.149.200.19.scholardev/files/list_of_pids.txt /var/www/html/drupal/sites/refworks_to_mods.xslt MODS TRUE',
    ),
    'aliases' => array('islandoraumxsl'),
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
function drush_islandora_update_metadata_from_xsl($list_file, $xslt_file, $datastream_id, $interactive) {
  drush_print('Current working directory ' . getcwd());
  if (isset($list_file)) {
    drush_print(" using list file" . $list_file);
  }
  else {
    drush_print(" no list of pids file found");
    return;
  }
  if (isset($xslt_file)) {
    drush_print(" using xslt file " . $xslt_file);
  }
  else {
    drush_print("no xslt file found");
    return;
  }

  islandora_update_metadata_from_xsl_go($list_file, $xslt_file, $datastream_id, $interactive);
}

/**
 * Iterates through all the objects returned by the itql query and updates the mods stream
 * 
 * @param string $query_file
 * @param string $substitution_text
 * @param boolean $interactive
 * 
 */
function islandora_update_metadata_from_xsl_go($list_file, $xslt_file, $datastream_id, $interactive = FALSE) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  ini_set("auto_detect_line_endings", "1");
  $xsl = file_get_contents($xslt_file);
  $pids = file($list_file);
  $xslt = new XSLTProcessor();
  $xslt->importStylesheet(new SimpleXMLElement($xsl));

  if (!isset($pids)) {
    drush_die('Could not read list of pids from file');
    return;
  }
  if (!isset($xsl)) {
    drush_die('Could not load contents of xslt file');
    return;
  }

  $num = count($pids);
  if (!drush_confirm(dt('are you sure you want to update all @num objects?', array('@num' => $num)))) {
    // was: return drush_set_error('CORE_SYNC_ABORT', 'Aborting.');
    drush_die('Aborting.');
  }
  $count = 0;
  if ($interactive == 'TRUE') {
    foreach ($pids as $object) {
      $object = trim($object);
      if (drush_confirm(dt('update %dsid in this @pid ?', array('@pid' => $object, '%dsid' => $datastream_id)))) {
        //$item = new Fedora_Item($object);
        islandora_update_datastream_from_xslt($object, $xslt, $datastream_id);
        drush_print("processed $object # " . $count++);
      }
    }
  }
  else {
    foreach ($pids as $object) {
      $object = trim($object);
      //$item = new Fedora_Item($object);
      islandora_update_datastream_from_xslt($object, $xslt, $datastream_id);
      drush_print("processed $object # " . $count++);
      if($count > 1000){
        drush_die('Reached limit of 1000 stopping');
      }
    }
  }
}

/**
 * update all the objects
 * 
 * @param string $pid
 * 
 */
function islandora_update_datastream_from_xslt($pid, $xslt, $dsid) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_item');


  $item = new Fedora_Item($pid);
  if (!isset($item)) {
    islandora_update_write_log(dt('error creating object %pid', array('%pid' => $pid)), 'islandora_update_from_xsl.log', 'error');
    return;
  }

  $metadata = $item->get_datastream_dissemination('refworks');
  $sxml = simplexml_load_string($metadata);
  if (!$sxml) {
    islandora_update_write_log(dt('error processing %pid', array('%pid' => $pid)), 'islandora_update_from_xsl.log', 'error');
    $item->forget();
    return;
  }
  $content = $xslt->transformToXml($sxml);
  //need to parse and add the doi easier in php then xslt
  $content = islandora_update_from_csv_process_xml($content, $metadata);
  //var_dump($content);
  if (isset($content)) {
    $value = $item->modify_datastream($content, $dsid, $dsid, 'text/xml', $force = FALSE, $logMessage = 'Modified by Islandora API using islandora_update_book_mods drush script', TRUE);
  }
  if (isset($value)) {
    islandora_update_write_log(dt('successfully updated %dsid for %pid', array('%dsid' => $dsid, '%pid' => $pid)), 'islandora_update_from_xsl.log');
  }
  else {
    islandora_update_write_log(dt('error updating %dsid for %pid', array('%dsid' => $dsid, '%pid' => $pid)), 'islandora_update_from_xsl.log', 'error');
  }
  time_nanosleep(0, 200000000);

  $item->forget();
}

/**
 * breask the refeworks <do> element at each semi colon and treats each as a seperate string
 * looks for DOI: doi: [DOI] [doi] [pii] uses the value as an attribute type and removes it from
 * the content of the element.
 * @param string $xml 
 */
function islandora_update_from_csv_process_xml($mods, $refworks) {
  //$content = $xslt->transformToXml($sxml);
  $refworks_xml = simplexml_load_string($refworks);
  $dois = $refworks_xml->do;
  $data = array();
  $mods_xml = simplexml_load_string($mods);
  foreach ($dois as $doi) {
    $data[] = explode(';', $doi);
  }
  foreach ($data as $identifier) {
    foreach ($identifier as $str) {
     if (stripos($str, '[pii]') !== FALSE) {
        $element = $mods_xml->addChild('identifier', trim(str_ireplace('[pii]', ' ', $str)));
        $element->addAttribute('type', 'pii');
      }
      elseif (stripos($str, 'doi:') !== FALSE) {
        $element = $mods_xml->addChild('identifier', trim(str_ireplace('doi:', ' ', $str)));
        $element->addAttribute('type', 'doi');
      }
      elseif (stripos($str, '[doi]') !== FALSE) {
        $element = $mods_xml->addChild('identifier', trim(str_ireplace('[doi]', ' ', $str)));
        $element->addAttribute('type', 'doi');
      }
      else {
        $element = $mods_xml->addChild('identifier', trim($str));
        $element->addAttribute('type', 'doi');
      }
    }
  }
  //&lt;I&gt;  &lt;/I&gt
  $titles = $mods_xml->titleInfo->title;
  islandora_update_from_xsl_fix_data($titles, array('<I>' => '','</I>' => '' ));
  
  $sub_titles = $mods_xml->titleInfo->subTitle;
  islandora_update_from_xsl_fix_data($sub_titles, array('<I>' => '','</I>' => ''));
  return $mods_xml->asXML();
}

/**
 *
 * @param array $nodes
 * @param array $strs 
 */
function islandora_update_from_xsl_fix_data($nodes, $strs){
  $keys = array_keys($strs);
  
  $vals = array_values($strs);
  
  foreach($nodes as $element){
    //drush_print('inside nodes');
   $new_str = str_replace($keys, $values, (string)$element );
   $new_str = htmlspecialchars_decode($new_str);
   $element[0] = $new_str;
  }
}

?>
