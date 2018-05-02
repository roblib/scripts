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

function islandora_get_extension_for_mimetype($mimetype) {
  $mimetypes = array(
    'x-world/x-3dmf' => '.qd3d',
    'application/octet-stream' => '.zoo',
    'application/x-authorware-bin' => '.aab',
    'application/x-authorware-map' => '.aam',
    'application/x-authorware-seg' => '.aas',
    'text/vnd.abc' => '.abc',
    'text/html' => '.htx',
    'video/animaflex' => '.afl',
    'application/postscript' => '.ps',
    'audio/x-aiff' => '.aiff',
    'application/x-aim' => '.aim',
    'text/x-audiosoft-intra' => '.aip',
    'application/x-navi-animation' => '.ani',
    'application/x-nokia-9000-communicator-add-on-software' => '.aos',
    'application/mime' => '.aps',
    'image/x-jg' => '.art',
    'video/x-ms-asf' => '.asf',
    'text/x-asm' => '.s',
    'text/asp' => '.asp',
    'video/x-ms-asf-plugin' => '.asx',
    'audio/x-au' => '.au',
    'video/x-msvideo' => '.avi',
    'video/avs-video' => '.avs',
    'application/x-bcpio' => '.bcpio',
    'application/x-macbinary' => '.bin',
    'image/bmp' => '.bm',
    'image/x-windows-bmp' => '.bmp',
    'application/book' => '.book',
    'application/x-bzip2' => '.bz2',
    'application/x-bsh' => '.bsh',
    'application/x-bzip' => '.bz',
    'text/plain' => '.txt',
    'application/vnd.ms-pki.seccat' => '.cat',
    'text/x-c' => '.cpp',
    'application/clariscad' => '.ccad',
    'application/x-cocoa' => '.cco',
    'application/x-netcdf' => '.nc',
    'application/x-x509-ca-cert' => '.der',
    'application/x-chat' => '.chat',
    'application/x-java-class' => '.class',
    'application/x-cpio' => '.cpio',
    'application/x-cpt' => '.cpt',
    'application/pkix-crl' => '.crl',
    'application/x-x509-user-cert' => '.crt',
    'text/x-script.csh' => '.csh',
    'text/css' => '.css',
    'application/x-director' => '.dxr',
    'application/x-deepv' => '.deepv',
    'video/x-dv' => '.dv',
    'video/x-dl' => '.dl',
    'application/msword' => '.word',
    'application/commonground' => '.dp',
    'application/drafting' => '.drw',
    'application/x-dvi' => '.dvi',
    'model/vnd.dwf' => '.dwf',
    'image/x-dwg' => '.svf',
    'text/x-script.elisp' => '.el',
    'application/x-elc' => '.elc',
    'application/x-envoy' => '.evy',
    'application/x-esrehber' => '.es',
    'text/x-setext' => '.etx',
    'text/x-fortran' => '.for',
    'application/vnd.fdf' => '.fdf',
    'image/fif' => '.fif',
    'video/x-fli' => '.fli',
    'image/florian' => '.turbot',
    'text/vnd.fmi.flexstor' => '.flx',
    'video/x-atomic3d-feature' => '.fmf',
    'image/vnd.net-fpx' => '.fpx',
    'application/freeloader' => '.frl',
    'audio/make' => '.pfunk',
    'image/g3fax' => '.g3',
    'image/gif' => '.gif',
    'video/x-gl' => '.gl',
    'audio/x-gsm' => '.gsm',
    'application/x-gsp' => '.gsp',
    'application/x-gss' => '.gss',
    'application/x-gtar' => '.gtar',
    'application/x-gzip' => '.gz',
    'multipart/x-gzip' => '.gzip',
    'text/x-h' => '.hh',
    'application/x-hdf' => '.hdf',
    'application/x-helpfile' => '.help',
    'application/vnd.hp-hpgl' => '.hpgl',
    'text/x-script' => '.hlb',
    'application/x-winhelp' => '.hlp',
    'application/x-mac-binhex40' => '.hqx',
    'application/hta' => '.hta',
    'text/x-component' => '.htc',
    'text/webviewhtml' => '.htt',
    'x-conference/x-cooltalk' => '.ice',
    'image/x-icon' => '.ico',
    'image/ief' => '.iefs',
    'model/iges' => '.igs',
    'application/x-ima' => '.ima',
    'application/x-httpd-imap' => '.imap',
    'application/inf' => '.inf',
    'application/x-internett-signup' => '.ins',
    'application/x-ip2' => '.ip',
    'video/x-isvideo' => '.isu',
    'audio/it' => '.it',
    'application/x-inventor' => '.iv',
    'i-world/i-vrml' => '.ivr',
    'application/x-livescreen' => '.ivy',
    'audio/x-jam' => '.jam',
    'text/x-java-source' => '.java',
    'application/x-java-commerce' => '.jcm',
    'image/pjpeg' => '.jpg',
    'image/jpeg' => '.jpg',
    'image/x-jps' => '.jps',
    'application/x-javascript' => '.js',
    'image/jutvision' => '.jut',
    'music/x-karaoke' => '.kar',
    'text/x-script.ksh' => '.ksh',
    'audio/x-nspaudio' => '.lma',
    'audio/x-liveaudio' => '.lam',
    'application/x-latex' => '.ltx',
    'application/x-lha' => '.lha',
    'text/x-script.lisp' => '.lsp',
    'text/x-la-asf' => '.lsx',
    'application/x-lzh' => '.lzh',
    'application/x-lzx' => '.lzx',
    'text/x-m' => '.m',
    'video/mpeg' => '.mpg',
    'audio/mpeg' => '.mpga',
    'audio/x-mpequrl' => '.m3u',
    'application/x-troff-man' => '.man',
    'application/x-navimap' => '.map',
    'application/mbedlet' => '.mbd',
    'application/x-magic-cap-package-1.0' => '.mc',
    'application/x-mathcad' => '.mcd',
    'text/mcf' => '.mcf',
    'application/netmc' => '.mcp',
    'application/x-troff-me' => '.me',
    'message/rfc822' => '.mhtml',
    'x-music/x-midi' => '.midi',
    'application/x-mif' => '.mif',
    'www/mime' => '.mime',
    'audio/x-vnd.audioexplosion.mjuicemediafile' => '.mjf',
    'video/x-motion-jpeg' => '.mjpg',
    'application/x-meme' => '.mm',
    'application/base64' => '.mme',
    'audio/x-mod' => '.mod',
    'video/quicktime' => '.qt',
    'video/x-sgi-movie' => '.mv',
    'video/x-mpeq2a' => '.mp2',
    'video/x-mpeg' => '.mp3',
    'application/x-project' => '.mpx',
    'application/vnd.ms-project' => '.mpp',
    'application/marc' => '.mrc',
    'application/x-troff-ms' => '.ms',
    'application/x-vnd.audioexplosion.mzz' => '.mzz',
    'image/naplps' => '.naplps',
    'application/vnd.nokia.configuration-message' => '.ncm',
    'image/x-niff' => '.niff',
    'application/x-mix-transfer' => '.nix',
    'application/x-conference' => '.nsc',
    'application/x-navidoc' => '.nvd',
    'application/oda' => '.oda',
    'application/x-omc' => '.omc',
    'application/x-omcdatamaker' => '.omcd',
    'application/x-omcregerator' => '.omcr',
    'text/x-pascal' => '.p',
    'application/x-pkcs10' => '.p10',
    'application/x-pkcs12' => '.p12',
    'application/x-pkcs7-signature' => '.p7a',
    'application/x-pkcs7-mime' => '.p7m',
    'application/x-pkcs7-certreqresp' => '.p7r',
    'application/pkcs7-signature' => '.p7s',
    'application/pro_eng' => '.prt',
    'text/pascal' => '.pas',
    'image/x-portable-bitmap' => '.pbm',
    'application/x-pcl' => '.pcl',
    'image/x-pict' => '.pct',
    'image/x-pcx' => '.pcx',
    'chemical/x-pdb' => '.xyz',
    'application/pdf' => '.pdf',
    'image/x-portable-greymap' => '.pgm',
    'image/pict' => '.pict',
    'application/x-newton-compatible-pkg' => '.pkg',
    'application/vnd.ms-pki.pko' => '.pko',
    'text/x-script.perl' => '.pl',
    'application/x-pixclscript' => '.plx',
    'text/x-script.perl-module' => '.pm',
    'application/x-pagemaker' => '.pm5',
    'image/png' => '.x-png',
    'image/x-portable-anymap' => '.pnm',
    'application/vnd.ms-powerpoint' => '.pwz',
    'model/x-pov' => '.pov',
    'image/x-portable-pixmap' => '.ppm',
    'application/x-mspowerpoint' => '.ppt',
    'application/mspowerpoint' => '.ppz',
    'application/x-freelance' => '.pre',
    'paleovu/x-pv' => '.pvu',
    'text/x-script.phyton' => '.py',
    'applicaiton/x-bytecode.python' => '.pyc',
    'audio/vnd.qcelp' => '.qcp',
    'image/x-quicktime' => '.qtif',
    'video/x-qtc' => '.qtc',
    'audio/x-realaudio' => '.ra',
    'audio/x-pn-realaudio' => '.rmm',
    'image/x-cmu-raster' => '.ras',
    'image/cmu-raster' => '.rast',
    'text/x-script.rexx' => '.rexx',
    'image/vnd.rn-realflash' => '.rf',
    'image/x-rgb' => '.rgb',
    'audio/mid' => '.rmi',
    'audio/x-pn-realaudio-plugin' => '.rpm',
    'application/vnd.nokia.ringing-tone' => '.rng',
    'application/vnd.rn-realplayer' => '.rnx',
    'application/x-troff' => '.tr',
    'image/vnd.rn-realpix' => '.rp',
    'text/vnd.rn-realtext' => '.rt',
    'text/richtext' => '.rtx',
    'video/vnd.rn-realvideo' => '.rv',
    'audio/s3m' => '.s3m',
    'aapplication/octet-stream' => '.saveme',
    'application/x-tbook' => '.tbk',
    'video/x-scm' => '.scm',
    'application/x-sdp' => '.sdp',
    'application/sounder' => '.sdr',
    'application/x-sea' => '.sea',
    'application/set' => '.set',
    'text/x-sgml' => '.sgml',
    'text/x-script.sh' => '.sh',
    'application/x-shar' => '.shar',
    'text/x-server-parsed-html' => '.ssi',
    'audio/x-psid' => '.sid',
    'application/x-stuffit' => '.sit',
    'application/x-koan' => '.skt',
    'application/x-seelogo' => '.sl',
    'application/smil' => '.smil',
    'audio/x-adpcm' => '.snd',
    'application/solids' => '.sol',
    'text/x-speech' => '.talk',
    'application/futuresplash' => '.spl',
    'application/x-sprite' => '.sprite',
    'application/x-wais-source' => '.wsrc',
    'application/streamingmedia' => '.ssm',
    'application/vnd.ms-pki.certstore' => '.sst',
    'application/step' => '.stp',
    'application/x-navistyle' => '.stl',
    'application/x-sv4cpio' => '.sv4cpio',
    'application/x-sv4crc' => '.sv4crc',
    'x-world/x-svr' => '.svr',
    'application/x-shockwave-flash' => '.swf',
    'application/x-tar' => '.tar',
    'text/x-script.tcl' => '.tcl',
    'text/x-script.tcsh' => '.tcsh',
    'application/x-tex' => '.tex',
    'application/x-texinfo' => '.texi',
    ' lication/x-texinfo' => '.texinfo',
    'application/x-compressed' => '.z',
    'image/x-tiff' => '.tiff',
    'audio/tsp-audio' => '.tsi',
    'audio/tsplayer' => '.tsp',
    'text/tab-separated-values' => '.tsv',
    'text/x-uil' => '.uil',
    'text/uri-list' => '.uris',
    'application/i-deas' => '.unv',
    'multipart/x-ustar' => '.ustar',
    'text/x-uuencode' => '.uue',
    'application/x-cdlink' => '.vcd',
    'text/x-vcalendar' => '.vcs',
    'application/vda' => '.vda',
    'video/vdo' => '.vdo',
    'application/groupwise' => '.vew',
    'video/vnd.vivo' => '.vivo',
    'application/vocaltec-media-desc' => '.vmd',
    'application/vocaltec-media-file' => '.vmf',
    'audio/x-voc' => '.voc',
    'video/vosaic' => '.vos',
    'audio/voxware' => '.vox',
    'audio/x-twinvq-plugin' => '.vql',
    'audio/x-twinvq' => '.vqf',
    'x-world/x-vrml' => '.wrz',
    'x-world/x-vrt' => '.vrt',
    'application/x-visio' => '.vsw',
    'application/wordperfect6.0' => '.wp5',
    'application/wordperfect6.1' => '.w61',
    'audio/x-wav' => '.wav',
    'application/x-qpro' => '.wb1',
    'image/vnd.wap.wbmp' => '.wbmp',
    'application/vnd.xara' => '.web',
    'application/x-123' => '.wk1',
    'windows/metafile' => '.wmf',
    'text/vnd.wap.wml' => '.wml',
    'application/vnd.wap.wmlc' => '.wmlc',
    'text/vnd.wap.wmlscript' => '.wmls',
    'application/vnd.wap.wmlscriptc' => '.wmlsc',
    'application/wordperfect' => '.wp6',
    'application/x-wpwin' => '.wpd',
    'application/x-lotus' => '.wq1',
    'application/x-wri' => '.wri',
    'text/scriplet' => '.wsc',
    'application/x-wintalk' => '.wtk',
    'image/xbm' => '.xbm',
    'video/x-amt-demorun' => '.xdr',
    'xgl/drawing' => '.xgz',
    'image/vnd.xiff' => '.xif',
    'application/excel' => '.xl',
    'application/x-msexcel' => '.xlw',
    'application/x-excel' => '.xlv',
    'audio/xm' => '.xm',
    'text/xml' => '.xml',
    'xgl/movie' => '.xmz',
    'application/x-vnd.ls-xpix' => '.xpix',
    'image/xpm' => '.xpm',
    'video/x-amt-showrun' => '.xsr',
    'image/x-xwindowdump' => '.xwd',
    'multipart/x-zip' => '.zip',
    'text/x-script.zsh)' => '.zsh',
    'application/rdf+xml' => '.xml',
    'application/xml' => '.xml',
    'image/tiff' => '.tiff',
  );

    return $mimetypes[$mimetype];
}

function deltree($dir) {
  $files = array_diff(scandir($dir), array('.','..'));
  foreach ($files as $file) {
    (is_dir("$dir/$file")) ? deltree("$dir/$file") : unlink("$dir/$file");
  }
  return rmdir($dir);
}