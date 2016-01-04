<?php

/*
 * This file should be put in the islandora modules plugins directory
 * typical usage: drush -u 1 islandora_gsearch_update path/to/query.txt gsearch_solr http://localhost:8080/fedoragsearch/rest TRUE
 * the above would give you an interactive update
 * if the last parameter is not TRUE then all the pids in the list will be indexed without prompting
 * 
 */

//drush hook

function roblib_scholar_update_xacml_drush_command() {
  $items = array();

  $items['roblib_scholar_update_xacml'] = array(
    'description' => "Removes the old XACML POLICY stream and creates a new one that the xacml editor understands.",
    'arguments' => array(
      'pidlist_file' => 'The path to a plain text file that contains a list of pids one pid per line',
     'interactive' => 'if TRUE then you will be asked to confirm the export for each object'
    ),
    'examples' => array(
      'drush -u 1 roblib_scholar_update_xacml path/to/pidlist.txt TRUE',
    ),
    'aliases' => array('rsux'),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_roblib_scholar_update_xacml($pidlist_file, $interactive) {
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

  roblib_scholar_update_xacml_doAction($pids, $interactive);
}

//just a function
function roblib_scholar_update_xacml_doAction($pids, $interactive) {

  foreach ($pids as $pid) {
    drush_print($pid);
  }
  $num = count($pids);
  if (!drush_confirm(dt('are you sure you want to index @num objects?', array('@num' => $num)))) {
    drush_die('Aborting.');
  }

  if ($interactive == 'TRUE') {
    foreach ($pids as $pid) {
      $pid = trim($pid);
      if (drush_confirm(dt('correct xacml for @pid ?', array('@pid' => $pid)))) {
        roblib_scholar_fix_xacml($pid);
      }
    }
  }
  else {
    foreach ($pids as $pid) {
      $pid = trim($pid);
      roblib_scholar_fix_xacml($pid);
    }
  }
}

function roblib_scholar_fix_xacml($pid) {
  $object = islandora_object_load($pid);
  module_load_include('inc', 'islandora_xacml_api', 'includes/xacml');
  $xacml = new Xacml();
  $xml = new DOMDocument();
  $test = $xml->loadXML($object['MODS']->getContent(''));
  if (empty($test)) {
    return -1;
  }
  $query = "//mods:identifier[@type='u1']";
  $xpath = new DOMXPath($xml);
  $xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  $results = $xpath->query($query, $xml);
  foreach ($results as $element) {
    // add xacml rules for u1 mods element if any exist
    $xacml->managementRule->addUser(array($element->nodeValue));
  }
  //$xacml->viewingRule->addRole(array('anonymous user', 'authenticated user'));
  $xacml->managementRule->addUser(array($object->owner, 'admin'));
  $xacml->managementRule->addRole(array('administrator', 'scholar editor'));
  $islandora_xacml = new IslandoraXacml($object, $xacml->getXmlString(FALSE));
  $islandora_xacml->writeBackToFedora();
  drush_print(dt('updated xacml for @object', array(
    '@object' => $pid)));

  $object = NULL;
  $xacml = NULL;
  $islandora_xacml = NULL;
  $xml = NULL;
  $xpath = NULL;

}




