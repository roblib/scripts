<?php

function islandora_newspapers_drush_command() {
  $items = array();

  $items['islandora_ingest_newspapers'] = array(
    'description' => "Creates the parent issue object and uploads the pages as child objects.  
      Requires the Drupal7 version of Islandora with tuque",
    'arguments' => array(
      'query_file' => 'The path to a CSV file that contains the information about the newspaper pages',
      'interactive' => 'if TRUE then you will be asked to confirm the update for each object',
      'reel_no' => 'The reel number',
    ),
    'examples' => array(
      'drush -u 1 --uri=http://localhost islandora_ingest_newspapers /path/to/pages.csv TRUE 1',
    ),
    'aliases' => array('islandoranews'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

function drush_islandora_newspapers_islandora_ingest_newspapers($query_file, $interactive = FALSE, $reel_no = NULL) {

  $path_name = dirname($query_file);
  
  if (!$reel_no) {
    $reel_number = drush_prompt(dt('Enter reel number'), NULL, TRUE);
  }

  $fh = fopen($query_file, 'r');
  $line_number = 0;

  while (($data = fgetcsv($fh)) !== FALSE) {
    $line_number++;
    if ($line_number != 1) {
      if ($line_number == 2 && $data[7] != 1) {
        $book_pid = drush_prompt(dt('Enter book pid'), NULL, TRUE);
      }
      if ($data[7] == '1') {
        //var_dump($data);
        $book_pid = create_book_object($data);
        drush_log(dt('Book PID: @book_pid', array('@book_pid' => $book_pid)), 'success');
        create_page_object($path_name, $data, $reel_no, $book_pid); 
      }
      else {
        create_page_object($path_name, $data, $reel_no, $book_pid);
      }
    }
  }
}

function create_book_object($data) {
  
  module_load_include('inc', 'islandora', 'RestConnection');
  //require_once "/var/www/html/drupal/sites/all/modules/islandora/RestConnection.inc";
  
  //$connection = new RestConnection();
  $tuque = islandora_get_tuque_connection();
  $connection = $tuque->connection;
  
  $date = $data[3];
  $title = $data[2];
  $volume = $data[4];
  $edition = $data[5];
  $issue = $data[6];
  $missing_pages = $data[10];

  $iso_date = $date[0] . $date[1] . $date[2] . $date[3] . '-' . $date[4] . $date[5] . '-' . $date[6] . $date[7] . 'T00:00:00.000Z';
 
  $non_sort_words = array('The', 'A', 'An');

  $title_array = explode(' ', $title);

  if (in_array($title_array[0], $non_sort_words)) {
    $non_sort = $title_array[0];
    unset($title_array[0]);
    $sort_title = implode(' ', $title_array);
  }
  else {
    $sort_title = $title;
  }

  $label = $title . ' -- ' . $date;

  $pid = 'guardian:' . $date;  

  $dsid = 'MODS';

  $content_model_pid = 'islandora:newspaperIssueCModel';

  $collection_pid = 'newspapers:guardian';
  
  $xml = <<<BOOKMODS
<mods xmlns="http://www.loc.gov/mods/v3" xmlns:mods="http://www.loc.gov/mods/v3" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
    <relatedItem type="host">
        <identifier type="lccn">cn 87080496</identifier>
        <part>
            <detail type="volume">
                <number>$volume</number>
            </detail>
            <detail type="issue">
                <number>$issue</number>
            </detail>
            <detail type="edition">
                <number>$edition</number>
                <caption></caption>
            </detail>
        </part>
    </relatedItem>
    <titleInfo>
        <nonSort>$non_sort</nonSort>
        <title>$sort_title</title>
    </titleInfo>
    <originInfo>
        <dateIssued encoding="iso8601">$iso_date</dateIssued>
    </originInfo>
    <identifier type="PID">$pid</identifier>
    <note type="missing">$missing_pages</note>
</mods>
BOOKMODS;
  
  $rdf_string = <<<BOOKRDF
  <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:fedora="info:fedora/fedora-system:def/relations-external#" xmlns:fedora-model="info:fedora/fedora-system:def/model#" xmlns:islandora="http://islandora.ca/ontology/relsext#">
  <rdf:Description rdf:about="info:fedora/$pid">
    <fedora-model:hasModel rdf:resource="info:fedora/$content_model_pid"></fedora-model:hasModel>
    <fedora:isMemberOfCollection rdf:resource="info:fedora/$collection_pid"></fedora:isMemberOfCollection>
  </rdf:Description>
</rdf:RDF>
BOOKRDF;
 
  $mods_xml = new DOMDocument();
  $mods_xml->loadXML($xml);

  $transform_xsl = new DOMDocument();
  $transform_xsl->load("mods_to_dc.xsl");

  $proc = new XSLTProcessor();
  $proc->importStylesheet($transform_xsl);
  $dc = $proc->transformToXML($mods_xml);

  $object = new NewFedoraObject($pid, $connection->repository);
  
  $object->label = $label; 
 
  $dc_datastream = new NewFedoraDatastream('DC', 'X', $object, $connection->repository);

  $dc_datastream->content = $dc;
  $dc_datastream->mimetype = 'text/xml';
  $dc_datastream->label = 'Dublin Core Record for this object';

  $object->ingestDatastream($dc_datastream);

  $mods_datastream = new NewFedoraDatastream('MODS', 'X', $object, $connection->repository);
  
  $mods_datastream->setContentFromString($xml);
  $mods_datastream->mimetype = 'text/xml';
  $mods_datastream->label = 'MODS record';
  $mods_datastream->checksum = TRUE;
  $mods_datastream->checksumType = 'MD5';
  $mods_datastream->logMessage = 'MODS datastream created using Newspapers batch ingest script || SUCCESS';
  
  $object->ingestDatastream($mods_datastream);
  
  $rels_datastream = new NewFedoraDatastream('RELS-EXT', 'X', $object, $connection->repository);
  $rels_datastream->setContentFromString($rdf_string);
  $rels_datastream->label = 'RELS-EXT';
  $rels_datastream->mimetype = 'text/xml';
  $rels_datastream->logMessage = 'RELS-EXT datastream created using Newspapers batch ingest script || SUCCESS';
  
  $object->ingestDatastream($rels_datastream);  
  try{
  $ingest = $tuque->repository->ingestObject($object);
  }catch(Exception $e){
    unset($object);
    throw($e);
  }
  
  
  if ($ingest == TRUE) {
    drush_log(dt("Book object @pid created successfully\n", array('@pid' => $pid)), 'success');
  }
  else {
    drush_log(dt("There was an error creating the book object!\n", 'warning'));
  }
  unset($object);
  unset($mods_datastream);
  unset($rels_datastream);
  unset($dc_datastream);
  
  return $pid;
}

function create_page_object($file_path, $data, $reel_number, $collection_pid) {

  $connection = islandora_get_tuque_connection();
 

  $page_number_short = $data[7];
  $page_number_leading_zeros = $data[8];
  $sequence_number = $data[9];
  $issue = $data[6];
  $volume = $data[4];
  $edition = $data[5];
  $date = $data[3];
  
  $file_name = $file_path . '/' . $data[1];
  if ($page_number_short == 1) {
  $rand = rand('100', '999');
  $tmp_file = '/tmp/tmp_file' . $rand . '.jpg';
    $command = 'convert -thumbnail 200x200 ' . $file_name . ' ' . $tmp_file;
    exec($command , $exec_output, $exec_return);
    if ($exec_return == 0) {
      $book_item = new FedoraObject($collection_pid, $connection->repository);
      $tn_datastream = new NewFedoraDatastream('TN', 'M', $book_item, $connection->repository);
      $tn_datastream->setContentFromFile($tmp_file, FALSE);
      $tn_datastream->label = 'Thumbnail';
      $tn_datastream->mimetype = 'image/jpg';
      $tn_datastream->checksum = TRUE;
      $tn_datastream->checksumType = 'MD5';
      $tn_datastream->logMessage = "TN derivative created by ImageMagick with command - $command || SUCCESS";
      $book_item->ingestDatastream($tn_datastream);
      unlink($tmp_file);
    }  
  }

  $label = 'Page ' . $page_number_short;

  $pid = 'guardian:' . $date . '-' . $page_number_leading_zeros;

  $dsid = 'MODS';

  //$content_model_pid = 'islandora:newspaperPageCModel';

  $xml = <<<PAGEMODS
<?xml version="1.0" encoding="UTF-8"?>
<mods xmlns="http://www.loc.gov/mods/v3" xmlns:mods="http://www.loc.gov/mods/v3" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
<titleInfo>
<title>Page $page_number_short</title>
</titleInfo>
    <part>
        <extent unit="pages">
            <start>$sequence_number</start>
        </extent>
        <detail type="page number">
            <number>$page_number_short</number>
        </detail>
    </part>
    <relatedItem type="original">
        <physicalDescription>
            <form type="microfilm"/>
        </physicalDescription>
        <identifier type="reel number">$reel_number</identifier>
        <identifier type="reel sequence number">$sequence_number</identifier>
        <location>
            <physicalLocation authority="marcorg" displayLabel="Source Repository">PCU</physicalLocation>
        </location>
    </relatedItem>
    <identifier type="PID">$pid</identifier>
    <note type="agencyResponsibleForReproduction" displayLabel="Institution Responsible for Digitization">PCU</note>
</mods>
PAGEMODS;
  
  $rdf_string = <<<RDF
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:fedora="info:fedora/fedora-system:def/relations-external#" xmlns:fedora-model="info:fedora/fedora-system:def/model#" xmlns:islandora="http://islandora.ca/ontology/relsext#">
<rdf:Description rdf:about="info:fedora/$pid">
        <hasModel xmlns="info:fedora/fedora-system:def/model#" rdf:resource="info:fedora/islandora:newspaperPageCModel"></hasModel>
        <isSequenceNumber xmlns="http://islandora.ca/ontology/relsext#">$page_number_short</isSequenceNumber>
        <isMemberOf xmlns="info:fedora/fedora-system:def/relations-external#" rdf:resource="info:fedora/$collection_pid"></isMemberOf>
        <isSection xmlns="http://islandora.ca/ontology/relsext#">1</fedora:isSection>
        <isPageOf xmlns="http://islandora.ca/ontology/relsext#" rdf:resource="inf:fedora/$collection_pid"></isPageOf>
        <isPageNumber xmlns="http://islandora.ca/ontology/relsext#">$page_number_short</isPageNumber>
</rdf:Description>
</rdf:RDF>
RDF;

  $mods_xml = new DOMDocument();
  $mods_xml->loadXML($xml);

  $transform_xsl = new DOMDocument();
  $transform_xsl->load("mods_to_dc.xsl");

  $proc = new XSLTProcessor();
  $proc->importStylesheet($transform_xsl);
  $dc = $proc->transformToXML($mods_xml);

  $object = new NewFedoraObject($pid, $connection->repository);

  $object->label = $label;

  $dc_datastream = new NewFedoraDatastream('DC', 'X', $object, $connection->repository);

  $dc_datastream->content = $dc;
  $dc_datastream->mimetype = 'text/xml';
  $dc_datastream->label = 'Dublin Core Record for this object';

  $object->ingestDatastream($dc_datastream);

  $mods_datastream = new NewFedoraDatastream('MODS', 'X', $object, $connection->repository);

  $mods_datastream->content = $xml;
  $mods_datastream->mimetype = 'text/xml';
  $mods_datastream->label = 'MODS record';
  $mods_datastream->checksum = TRUE;
  $mods_datastream->checksumType = 'MD5';
  $mods_datastream->logMessage = 'MODS datastream created using Newspapers batch ingest script || SUCCESS';

  $object->ingestDatastream($mods_datastream);
  
  $image_datastream = new NewFedoraDatastream('TIFF', 'M', $object, $connection->repository);
  $image_datastream->setContentFromFile($file_name, FALSE);
  $image_datastream->label = 'TIFF image';
  $image_datastream->mimetype = 'image/tiff';
  $image_datastream->checksum = TRUE;
  $image_datastream->checksumType = 'MD5';
  $image_datastream->logMessage = 'TIFF datastream created using Newspapers batch ingest script || SUCCESS';
  
  $object->ingestDatastream($image_datastream);
  
  $rels_datastream = new NewFedoraDatastream('RELS-EXT', 'X', $object, $connection->repository);
  $rels_datastream->setContentFromString($rdf_string);
  $rels_datastream->label = 'Fedora Object to Object Relationship Metadata.';
  $rels_datastream->mimetype = 'text/xml';
  $rels_datastream->logMessage = 'RELS-EXT datastream created using Newspapers batch ingest script || SUCCESS';
  
  $object->ingestDatastream($rels_datastream);
  try{
  $ingest = $connection->repository->ingestObject($object);
  }catch(Exception $e){
    unset($object);
    throw($e);
  }

  if ($ingest == TRUE) {
    drush_log(dt("Page object @pid created successfully\n", array('@pid' => $pid)), 'success');
  }
  else {
    drush_log(dt("There was an error creating the page object!\n"), 'error');
  }
  
  unset($object);//make sure destructors are called as this script can run for a long time.
  unset($image_datastream);
  unset($mods_datastream);
  unset($rels_datastream);
  unset($dc_datastream);
  

  return $pid;
}

?>
