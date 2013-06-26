<?php

/*
 * put this file where ever you put your drush commands
 * 
 *
 * 
 */

//drush hook

function islandora_image_batch_drush_command() {
  $items = array();

  $items['islandora_image_batch'] = array(
    'description' => "scans a directory for files with a given mimetype and ingests them into the collection provided",
    'arguments' => array(
      'directory' => 'The path to a directory that contains image files that have extension like .jpg, .gif or .png files',
      'mimetype' => 'the mimetype accepted, should correspond to the files in the directory specified above',
      'collection' => 'the pid of the object that these will be related',
      'pid_namespace' => 'the pid namespace for the new objects',
      'cmodel' => 'the cmodel for the object',
      'dsid' => 'the dsid for the datastream',
    ),
    'examples' => array(
      'drush -u 1 --uri=http://137.149.200.19/scholardev islandoraib /home/user/dir/of/images/ image/png islandora:sp_basic_image_collection islandora islandora:sp_basic_image TIFF',
    ),
    'aliases' => array('islandoraib'),
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
function drush_islandora_image_batch($directory, $mimetype = 'image/jpeg'
, $collection = 'islandora:sp_basic_image_collection', $pid_namespace = 'islandora'
, $cmodel = 'islandora:sp_basic_image', $dsid = 'OBJ') {
  drush_print('Current working directory ' . getcwd());
  if (isset($directory)) {
    drush_print(" using directory" . $directory);
  }
  islandora_image_batch_go($directory, $mimetype, $collection
      , $pid_namespace, $cmodel, $dsid);
}

/**
 * Iterates through all the files that match the given mimetype
 *
 */
function islandora_image_batch_go($directory, $mimetype, $collection
, $pid_namespace, $cmodel, $dsid) {
  $files = scandir($directory);
  foreach ($files as $file) {
    if (mime_content_type($directory . '/' . $file) == $mimetype) {
      islandora_image_batch_ingest($directory, $file, $mimetype, $collection
          , $pid_namespace, $cmodel, $dsid);
    }
  }
}

function islandora_image_batch_ingest($directory, $file, $mimetype, $collection
, $pid_namespace, $cmodel, $dsid) {
  $connection = islandora_get_tuque_connection();
  $repository = $connection->repository;
  $pid = $repository->api->m->getNextPid($pid_namespace);
  $object = new NewFedoraObject($pid, $repository);
  $object->label = $file;
  $obj_datastream = new NewFedoraDatastream($dsid, 'M', $object, $repository);
  $obj_datastream->setContentFromFile($directory . '/' . $file, FALSE);
  $obj_datastream->label = $dsid;
  $obj_datastream->mimetype = $mimetype;
  $obj_datastream->checksum = TRUE;
  $obj_datastream->checksumType = 'MD5';
  $obj_datastream->logMessage = "added by drush script";
  $object->ingestDatastream($obj_datastream);

  $xml = <<<PAGEMODS
<?xml version="1.0" encoding="UTF-8"?>
<mods xmlns="http://www.loc.gov/mods/v3" xmlns:mods="http://www.loc.gov/mods/v3" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
<titleInfo>
<title>$file</title>
</titleInfo>
    <identifier type="PID">$pid</identifier>
</mods>
PAGEMODS;

  $mods_datastream = new NewFedoraDatastream('MODS', 'X', $object, $connection->repository);
  $mods_datastream->content = $xml;
  $mods_datastream->mimetype = 'text/xml';
  $mods_datastream->label = 'MODS record';
  $mods_datastream->checksum = TRUE;
  $mods_datastream->checksumType = 'MD5';
  $mods_datastream->logMessage = 'MODS datastream created using islandora drush image batch ingest script';
  $object->ingestDatastream($mods_datastream);

  $rdf_string = <<<RDF
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:fedora="info:fedora/fedora-system:def/relations-external#" xmlns:fedora-model="info:fedora/fedora-system:def/model#" xmlns:islandora="http://islandora.ca/ontology/relsext#">
<rdf:Description rdf:about="info:fedora/$pid">
        <fedora-model:hasModel  rdf:resource="info:fedora/$cmodel"></fedora-model:hasModel>
        <fedora:isMemberOfCollection  rdf:resource="info:fedora/$collection"></fedora:isMemberOfCollection>
</rdf:Description>
</rdf:RDF>
RDF;

  $rels_datastream = new NewFedoraDatastream('RELS-EXT', 'X', $object, $connection->repository);
  $rels_datastream->setContentFromString($rdf_string);
  $rels_datastream->label = 'Fedora Object to Object Relationship Metadata.';
  $rels_datastream->mimetype = 'text/xml';
  $rels_datastream->logMessage = 'RELS-EXT datastream created using islandora drush image batch ingest script';

  $object->ingestDatastream($rels_datastream);


  try{
    $ingest = $repository->ingestObject($object);
    drush_print("successfully processed $pid ");
  } catch (Exception $e){
    unset($object);
    unset($rels_datastream);
    unset($mods_datastream);
    unset($obj_datastream);
    drush_print("failed processing rels of $pid # " . $e->getMessage());
  }
  unset($object);
  unset($rels_datastream);
  unset($mods_datastream);
  unset($obj_datastream);
}

?>
