<?php


function islandora_ingest_objects_drush_command() {
  $items = array();

  $items['islandora-ingest-foxml-files'] = array(
      
    'aliases' => array('ingest-file'),
    'description' => 'Ingest FOXML files into the site\'s repository.',
    'options' => array(
      'fedora-user' => array(
        'description' => 'Fedora user (default \'fedoraAdmin\')',
      ),
      'fedora-pass' => array(
        'description' => 'Fedora repository password',
      ),
      'fedora-url' => array(
        'description' => 'Fedora repository URL. (default \'http://localhost:8080/fedora\')',
      ),
      'foxml-dir' => array(
        'description' => 'Absolute path to the directory to load all files from.',
        'required' => TRUE,
      ),
      'foxml-files' => array(
        'description' => 'A comma-delimited list of FOXML files located in the path specified by foxml-dir. If not given then all files with a .xml extension in that directory will be ingested.',
      ),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );

  return $items;
}

function drush_islandora_ingest_objects_islandora_ingest_foxml_files() {
  $dir = rtrim(drush_get_option('foxml-dir', '/tmp'), '/');
  if (empty(drush_get_option('foxml_files')) && drush_get_option('foxml-dir')) {
    $files = glob($dir . '/*.xml');
  }
  else {
    $files = preg_filter('/^/', $dir . '/', drush_get_option('foxml-files'));
  }
  if (empty($files)) {
    drush_set_error('No files found in ' . $dir);
    drush_die('Aborting');
  }

  if (!empty(drush_get_option('fedora-user', '')) && !empty(drush_get_option('fedora-pass', ''))) {
    $fakeuser = new StdClass();

    $fakeuser->uid = 1;
    $fakeuser->name = drush_get_option('fedora-user', $fakeuser->name);
    $fakeuser->pass = drush_get_option('fedora-pass', $fakeuser->pass);
    $tuque = islandora_get_tuque_connection($fakeuser);
  }
  else {
    $tuque = islandora_get_tuque_connection();
  }
  $connection = $tuque->connection;

  $serializer = new FedoraApiSerializer();

  $apim = new FedoraApiM($connection, $serializer);

  foreach ($files as $file) {
    try {
      $pid = $apim->ingest(array('file' => $file));
      drush_log("Ingested PID " . $pid, 'ok');
    }
    catch (\RepositoryException $e) {

      drush_log($e->getMessage(), 'error');
    }
  }


}














