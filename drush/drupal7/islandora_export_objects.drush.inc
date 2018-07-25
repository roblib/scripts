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
function islandora_export_objects_drush_command() {
  $items = array();

  $items['islandora-export-objects'] = array(
    'description' => "exports foxml for each object returned by a query, files will be located in the sites temp dir.
      Currently relies on the Drupal 6 version of Islandora",
    'options' => array(
      'pids' => array(
        'description' => 'Comma-separated list of PIDs to migrate',
      ),
      'pids-file-dir' => array(
        'description' => 'Path to PIDs file',
      ),
      'pids-filename' => array(
        'description' => 'Filename, excluding the path, of the PIDs file',
      ),
      'include-datastreams' => array(
        'description' => 'TRUE to export each objects datastreams in a folder named for the object\'s PID',
      ),
      'include-datastream-file-extensions' => array(
        'description' => 'TRUE to append file extensions to datastreams. This breaks re-import but lets you load the ifle smore easily in other programs.'
      ),
    ),
    'examples' => array(
      'drush islandora_export_objects /path/to/query.txt TRUE',
    ),
    'aliases' => array('islandoraexp'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  $items['islandora-import-objects'] = array(
    'description' => "Create an object and import the datastreams in a folder resulting from islandora-export-objects command.",

    'options' => array(
      'foxml-files' => array(
        'description' => 'Comma-separated list of FOXML to clone',
      ),
      'foxml-files-list' => array(
        'description' => 'File contianing list of files to import.'
      ),
      'foxml-files-dir' => array(
        'description' => 'Path to PIDs file',
      ),
      'use-exported-datastreams' => array(
        'description' => 'TRUE to load datastreams exported by the islandora-export-datastreams command',
      ),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN, // we can pass in users id on the command line using drush -u.
  );

  return $items;
}

//drush hook
function drush_islandora_export_objects() {
  $pids = drush_islandora_export_get_pids_from_params();

  $location = drush_get_option('pids-file-dir');

  if (!isset($pids)) {
    drush_die('no objects found');
    return;
  }

  global $user;
  $tuque = islandora_get_tuque_connection($user);
  $repository = $tuque->repository;

  foreach ($pids as $pid) {
    $pid = trim($pid);
    $foxml = $repository->api->m->export($pid, array('context' => 'migrate'));
    $file_base_name = str_replace(':', '_', $pid);
    $file_name = $location . '/' . $file_base_name . '.xml';
    drush_print('saving file to ' . $file_name);
    file_put_contents($file_name, $foxml);
    if (drush_get_option('include-datastreams')) {
      try {
        $datastreams = $repository->api->a->listDatastreams($pid);
        $fp = fopen($location . '/' . $file_base_name . '.json', 'w');
        fwrite($fp, json_encode($datastreams, JSON_PRETTY_PRINT));
        fclose($fp);
        $ds_path = $location . '/' . $file_base_name;
        if (file_exists($ds_path)) {
          deltree($ds_path);
        }
        mkdir($location . '/' . $file_base_name);

        foreach ($datastreams as $dsid => $datastream) {

          $ext = drush_get_option('include-datastream-file-extensions') ? islandora_get_extension_for_mimetype($datastream['mimetype']) : '';

          $ds_filename = $location . '/' . $file_base_name . '/' . $dsid . $ext;

          $ds_profile = $repository->api->m->getDatastream($pid, $dsid);
          $ds_profile_filename = $location . '/' . $file_base_name . '/' . $dsid . '.profile.json';
          $fp = fopen($ds_profile_filename, 'w');
          fwrite($fp, json_encode($ds_profile, JSON_PRETTY_PRINT));
          fclose($fp);
          $repository->api->a->getDatastreamDissemination($pid, $dsid, NULL, $ds_filename);

        }

      }
      catch(\Exception $e) {
        drush_log($e->getMessage());
        continue;
      }
    }
  }

  drush_print('files are located in ' . $location);
}

function drush_islandora_export_export_datastream($location, $file_base_name, $datastream) {

}

function drush_islandora_export_get_pids_from_params() {

  if (!empty(drush_get_option('pids'))) {
    return explode(',', drush_get_option('pids'));
  }
  else if (!empty(drush_get_option('pids-file-dir')) && !empty(drush_get_option('pids-filename'))) {
    $dir = rtrim(drush_get_option('pids-file-dir', '/tmp'), '/');
    $pids_filename = $dir . '/' . drush_get_option('pids-filename');

    $pids = explode("\n", file_get_contents($pids_filename));

    return $pids;
  }
  return NULL;
}

function drush_islandora_export_objects_islandora_import_objects() {
  module_load_include('inc', 'islandora', 'includes/utilities');
  if (drush_get_option('foxml-files')) {
    $foxml_files = explode(',', drush_get_option('foxml-files'));
  }
  else {
    $foxml_files = explode("\n", file_get_contents(drush_get_option('foxml-files-list')));
  }
  $tuque = islandora_get_tuque_connection();

  $connection = $tuque->connection;
  $serializer = new FedoraApiSerializer();

  $source_apim = new FedoraApiM($connection, $serializer);

  foreach($foxml_files as $foxml_file) {
    if (empty($foxml_file)) {
      continue;
    }
    $path = rtrim(drush_get_option('foxml-files-dir', '/')) . '/' . $foxml_file;
    $path_info = pathinfo($path);
    if (file_exists($path)) {
      $foxml = simplexml_load_file($path);
      $foxml->registerXPathNamespace('foxml', "info:fedora/fedora-system:def/foxml#");
      $result = $foxml->xpath("/foxml:digitalObject/@PID");
      $pid = (string)$result[0];
      if (!empty($pid)) {
        $property_names = array_map('drush_islandora_export_objects_strip_fedora_prefix', array_map('strval', $foxml->xpath("/foxml:digitalObject/foxml:objectProperties/foxml:property/@NAME")));
        $property_values = array_map('strval', $foxml->xpath("/foxml:digitalObject/foxml:objectProperties/foxml:property/@VALUE"));
        $properties = array_combine($property_names, $property_values);
        $datastreams = drush_islandora_export_objects_get_datastreams_for_object($pid, drush_get_option('foxml-files-dir'), $foxml_file, $path_info['filename']);
        $object = islandora_prepare_new_object($pid, $properties['label'], $datastreams);
        try {
          $tuque->repository->ingestObject($object);
          drush_log("Ingested $pid.", 'ok');
        }
        catch (\Exception $e) {
          drush_log($e->getmessage, 'error');
        }
      }
    }
  }
}

function drush_islandora_export_objects_strip_fedora_prefix($str) {
  return explode('#', $str)[1];
}

function drush_islandora_export_objects_get_datastreams_for_object($pid, $dir, $foxml_file, $base_filename) {
  $datastreams = array();
  $path_info = pathinfo($dir . '/' . $foxml_file);
  $ds_path = $path_info['dirname'] . '/' . $path_info['filename'];
  $ds_list_file = $ds_path . '.json';
  if (file_exists($ds_path) && file_exists($ds_list_file)) {
    $ds_list = array_keys(json_decode(file_get_contents($ds_list_file), TRUE));
    foreach ($ds_list as $ds) {
      $ds_array = array();
      $ds_array['dsid'] = $ds;
      $ds_json = json_decode(file_get_contents($dir . '/' . $base_filename . '/' . $ds . '.profile.json'), TRUE);
      $ds_array['label'] = $ds_json['dsLabel'];
      $ds_array['mimetype'] = $ds_json['dsMIME'];
      $ds_array['control_group'] = $ds_json['dsControlGroup'];
      $ds_array['datastream_file'] = $dir . '/' . $base_filename . '/' . $ds;
      $datastreams[] = $ds_array;
    }
  }
  return $datastreams;
}

/**
 * returns an array of pids
 * @todo pull this up into an api
 * @param SimpleXMLElement $content
 * @return array
 */
function islandora_export_sparql_results_as_array($results) {
    $resultsarray = array();
    foreach($results as $result){
        // var_dump($result);
        $resultsarray[] = $result['object']['value'];
    }
    return $resultsarray;
}

function deltree($dir) {
  $files = array_diff(scandir($dir), array('.','..'));
  foreach ($files as $file) {
    (is_dir("$dir/$file")) ? deltree("$dir/$file") : unlink("$dir/$file");
  }
  return rmdir($dir);
}