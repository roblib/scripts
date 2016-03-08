<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 islandora_gsearch_update path/to/query.txt gsearch_solr http://localhost:8080/fedoragsearch/rest TRUE
 * the above would give you an interactive update
 * if the last parameter is not TRUE then all the pids in the list will be indexed without prompting
 * 
 */

//drush hook

function roblib_scholar_fix_mods_drush_command() {
  $items = array();

  $items['roblib_scholar_fix_mods'] = array(
    'description' => "update the mods so it conforms to new islandscholar expectations (and fix some common errors in our mods).",
    'arguments' => array(
      'pidlist_file' => 'The path to a plain text file that contains a list of pids one pid per line',
      'interactive' => 'if TRUE then you will be asked to confirm the export for each object'
    ),
    'examples' => array(
      'drush -u 1 roblib_scholar_fix_mods path/to/pidlist.txt TRUE',
    ),
    'aliases' => array('rsfm'),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_roblib_scholar_fix_mods($pidlist_file, $interactive) {
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

  roblib_scholar_fix_mods_doAction($pids, $interactive);
}

//just a function
function roblib_scholar_fix_mods_doAction($pids, $interactive) {

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
      if (drush_confirm(dt('correct mods for @pid ?', array('@pid' => $pid)))) {
        roblib_scholar_fix_mods($pid);
      }
    }
  }
  else {
    foreach ($pids as $pid) {
      $pid = trim($pid);
      roblib_scholar_fix_mods($pid);
    }
  }
}

/**
 * Update the citation and thesis cmodel namespaces.
 * @param string $pid
 * @return int
 *   return FALSE if we could not read and update the MODS TRUE on success
 */
function roblib_scholar_fix_mods($pid) {
  $object = islandora_object_load($pid);
  if(empty($object)){
    drush_print(dt('Could not load object @pid, cannot update',
      array('@pid' => $pid)));
    return FALSE;
  }
  $mods = $object['MODS']->content;
  if (empty($mods)) {
    drush_print(dt('no MODS datastream found for for object @pid, cannot update',
      array('@pid' => $pid)));
    return FALSE;
  }
  $mods_xml_doc = new DOMDocument();
  $mods_xml_doc->loadXML($mods);
  $is_namepart_modified = roblib_scholar_fix_author($mods_xml_doc);
  $is_part_moved = roblib_scholar_move_part($mods_xml_doc);
  $is_schema_location_modified = roblib_scholar_update_schema_loc($mods_xml_doc);
  if ($is_namepart_modified || $is_part_moved || $is_schema_location_modified) {
    $raw_xml = $mods_xml_doc->saveXML($mods_xml_doc);
    $object['MODS']->content = $raw_xml;
    drush_print(dt("modified MODS of @pid", array(
      '@pid' => $pid,
    )));
  } else {
    drush_print(dt("DID NOT modify MODS of @pid", array(
      '@pid' => $pid,
    )));
  }

  unset($object);

  return TRUE;
}

/**
 * Change the xsi:schemaLocation, many objects point to a vre2 url which may not exist in the future.
 *
 * @param DomDocument $mods_xml_doc
 *
 * @return bool
 *   TRUE if the mods has been modified.
 */
function roblib_scholar_update_schema_loc(&$mods_xml_doc) {
  $is_modified = FALSE;
  $query = "//mods:mods";
  $xpath = new DOMXPath($mods_xml_doc);
  $xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  $results = $xpath->query($query, $mods_xml_doc);
  if(!$results->length > 0){
    return FALSE;
  }
  $mods = $results->item(0);
  $schema_location = $mods->getAttribute("xsi:schemaLocation");
  if($schema_location == "http://www.loc.gov/mods/v3 http://www.islandscholar.ca/sites/vre2.upei.ca.islandscholar/files/mods-3-4.xsd"){
    $schema_location = "http://www.loc.gov/mods/v3 http://www.loc.gov/standards/mods/v3/mods-3-4.xsd";
    $mods->removeAttribute('xsi:schemaLocation');
    $mods->setAttribute('xsi:schemaLocation', $schema_location);
    $is_modified = TRUE;
    drush_print("Modified schema location");
  }
  return $is_modified;
}

/**
 * Correct the author, editor, contributor roleterm.
 * Make sure the type is text and the initial letter is capitalized.
 *
 * @param DOMDocument $mods_xml_doc
 * @return bool
 *   returns TRUE if the xml was modified
 */
function roblib_scholar_fix_author(&$mods_xml_doc) {
  $is_modified = FALSE;
  $query = "//mods:name[@type='personal']/mods:role/mods:roleTerm";
  $xpath = new DOMXPath($mods_xml_doc);
  $xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  $results = $xpath->query($query, $mods_xml_doc);
  foreach ($results as $result) {
    // fix for old IslandScholar mods, if you have nameparts that have two roleterms with type = code and type = text then
    // this section would need to be modified we assume only one roleterm per namepart exists
    //if (substr($result->nodeValue, 0, strlen('aut')) === 'aut') {
    if ($result->nodeValue === 'aut' || $result->nodeValue === 'Author') {
      $result->nodeValue = 'author';
      $is_modified = TRUE;
    }
    //elseif (substr($result->nodeValue, 0,strlen('ed')) === 'ed') {
    elseif ($result->nodeValue === 'edt' || $result->nodeValue === 'Editor') {
      $result->nodeValue = 'editor';
      $is_modified = TRUE;
    }
    elseif ($result->nodeValue === 'Contributor') {
      $result->nodeValue = 'contributor';
      $is_modified = TRUE;
    }

    if ($result->getAttribute('type') != 'text') {
      $result->setAttribute('type', 'text');
      $is_modified = TRUE;
    }
    if ($result->getAttribute('authority') != 'local') {
      $result->setAttribute('authority', 'local');
      $is_modified = TRUE;
    }
  }
  if ($is_modified) {
    drush_print("Modified name element");
  }
  return $is_modified;
}

function roblib_scholar_move_part(&$mods_xml_doc) {
  // if this already has a mods->part do nothing
  $query = "//mods:mods/mods:part";
  $xpath = new DOMXPath($mods_xml_doc);
  $xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  $results = $xpath->query($query, $mods_xml_doc);
  if (!empty($results->item(0))) {
    return FALSE;
  }
  // only do this for host not series or others
  $query = "//mods:relatedItem[@type='host']/mods:part";
  //$xpath = new DOMXPath($mods_xml_doc);
  //$xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  $results = $xpath->query($query, $mods_xml_doc);
  if (empty($results->item(0))) {
    return FALSE;
  }
  $part = $results->item(0);
  $mods = $part->parentNode->parentNode;
  $mods->appendChild($part);
  drush_print("moved part element");
  return TRUE;
}





