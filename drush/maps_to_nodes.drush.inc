<?php
/**
 *Map maps to drupal nodes 
 * syncs fedora map cmodel to drupal content.
 */
 function maps_to_nodes_drush_command(){
   $items = array();
   $items['maps-to-nodes'] = array(
        'callback' => 'drush_maps_to_nodes',
        'description' => "create nodes/entities from key fields from Map cmodel.  
          Requires Drupal7 version of Islandora with tuque",
        'aliases' => array('mapstonodes'),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
        'arguments' => array(
            'query_file' => 'The full path to a file that contains an itql query',
            'pid' => 'Fedora PID of a collection.  Currently only used to get a relationship object.  
              Should be refactored.',
        ),
        'examples' => array(
            'drush maps-to-nodes path/to/itql/textfile.txt',
        ),
    );
   return $items;
 }
 
 //drush hook
 /** default query
  *select $object from <#ri> where 
        ($object <fedora-rels-ext:isMemberOfCollection> <info:fedora/imagined:collection> or
        $object <fedora-rels-ext:isMemberOf> <info:fedora/imagined:collection>) 
        and $object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>
        and $object <fedora-model:hasModel> <info:fedora/islandora:mapCModel>
  * @param type $directory
  * @param type $interactive 
  */
function drush_maps_to_nodes($query_file, $collection_pid) {
    drush_print('Current working directory ' . getcwd());
    $query = file_get_contents($query_file);
    drush_print($query);
    $item = islandora_object_load($collection_pid);
    $results = $item->repository->ri->query($query, 'itql');
    maps_to_nodes_create_nodes($results);
    drush_print($item->label);
    //scan_directory($directory);
}

function maps_to_nodes_create_nodes($results){
  $count = 0;
  $limit = 100;//hardcoded for testing purposes but we could pass in a parameter if we want to have limit capability
  foreach ($results as $result){
    //drush_print($result['object']['value']);
    $item = islandora_object_load($result['object']['value']);
    maps_to_nodes_create_node($item);
    if($count++ > $limit){
      drush_print("reached max limit of $count");
      return;
    }
  }
  
}

function maps_to_nodes_create_node(&$item){
  $mods_string = $item['MODS']->content;
   $modsXml = new SimpleXMLElement($mods_string);
    $modsXml->registerXPathNamespace("mods", "http://www.loc.gov/mods/v3");
    $title = $modsXml->xpath('/mods:mods/mods:titleInfo/mods:title');
    $note = $modsXml->xpath('//mods:note[@type="details"]');
    $physical_description = $modsXml->xpath('//mods:physicalDescription/mods:extent');
    $geographic_country = $modsXml->xpath('//mods:country');
    $geographic_province = $modsXml->xpath('//mods:province');
    $geographic_county = $modsXml->xpath('//mods:county');
    $geographic_region = $modsXml->xpath('//mods:region');
    $access_reproduction = $modsXml->xpath('//mods:accessCondition[@type="useAndReproduction"]');
    $access_restriction = $modsXml->xpath('//mods:accessCondition[@type="restrictionOnAccess"]');
    $physical_location = $modsXml->xpath('//mods:physicalLocation');
    $accession_number = $modsXml->xpath('//mods:identifier[@type="paro"]');
    $name = $modsXml->xpath('//mods:name[@type="personal"]/mods:namePart');
    $date_created = $modsXml->xpath('//mods:dateCreated');
    $formatted_date_created = NULL;
    if(!empty($date_created[0])){
      $formatted_date_created = maps_to_nodes_trim($date_created[0], 4);
      $formatted_date_created =  $formatted_date_created . '-00-00T00:00:00' ;
    }
    $topics = $modsXml->xpath('//mods:topic');
    $node = new stdClass();
    $node->type= 'maps';
    node_object_prepare($node);
    $node->title = (string)$title[0];
    $node->language = LANGUAGE_NONE;
    $node->body[$node->language][0]['value']   = (string)$note[0];
    $node->field_mods_name[$node->language][0]['value'] = (string)$name[0];
    $node->field_mods_abstract[$node->language][0]['value'] = (string)$note[0];
    $node->field_mods_identifier[$node->language][0]['value'] = $item->id;
    //$node->field_date_created[$node->language][0]['value'] = date();
    $node->field_mods_physical_description[$node->language][0]['value'] = maps_to_nodes_trim((string)$physical_description[0],254);
    //$node->field_mods_geographic_country[$node->language][0]['value'] = (string)$geographic_country[0];
    maps_to_nodes_populate_multi($node, "field_mods_geographic_country", $geographic_country);
    //$node->field_mods_geographic_province[$node->language][0]['value'] = (string)$geographic_province[0];
    maps_to_nodes_populate_multi($node, "field_mods_geographic_province", $geographic_province);
    //$node->field_mods_geographic_county[$node->language][0]['value'] = (string)$geographic_county[0];
    maps_to_nodes_populate_multi($node, "field_mods_geographic_county", $geographic_county);
    //$node->field_mods_geographic_region[$node->language][0]['value'] = (string)$geographic_region[0];
    maps_to_nodes_populate_multi($node, 'field_mods_geographic_region', $geographic_region);
    maps_to_nodes_populate_multi($node, 'field_mods_subject', $topics);
    $node->field_mods_access_reproduction[$node->language][0]['value'] = maps_to_nodes_trim((string)$access_reproduction[0],254);
    $node->field_mods_access_restriction[$node->language][0]['value'] = maps_to_nodes_trim((string)$access_restriction[0],254);
    $node->field_mods_location_physical[$node->language][0]['value'] = maps_to_nodes_trim((string)$physical_location[0],254);
    $node->field_accession_number[$node->language][0]['value'] = (string)$accession_number[0];
    $node->field_mods_date_created[$node->language][0]['value'] = (string)$date_created[0];
    if(isset($formatted_date_created)){
      drush_print('Adding date '. $formatted_date_created);
      $node->field_formatted_date_created[$node->language][0]['value'] = $formatted_date_created;
    }
    maps_to_nodes_populate_image_field($node, $item);
   node_save($node);
    drush_print('Added node with title '. (string)$title[0]);
    //unset($item);//may need to do this for large object setss
}
function maps_to_nodes_trim($string, $length){
  if(strlen($string)>$length){
    $string = substr($string,0,$length);
  }
  return $string;
}

function maps_to_nodes_populate_image_field(&$node, $item){  
  $file_name = str_ireplace(':', '_', $item->id);
  $file = file_save_data($item['JPG']->content, 'public://' . $file_name . '.jpg');
  $dest = file_default_scheme() . "://image";
  $file = file_copy($file, $dest);
  $node->field_islandora_med_img[$node->language][0] = (array)$file;  
}

function maps_to_nodes_populate_multi(&$node, $field, $xml_node){
  $count = 0;
  foreach($xml_node as $element){
    $node->{$field}[$node->language][$count]['value'] = maps_to_nodes_trim((string)$element[$count++],254);
  }
}
?>
