<?php
function roblib_perms_drush_command(){
  $items['list-unused-perms'] = array(
    'description' => 'list permissions a role does not have.',
    'examples' => array(
      "drush list-unused-perms 'test role'" => "List all permissions this role does not have.",
      ),
    'arguments' => array(
      'machine name' => 'The symbolic machine name for the role. Required.',

    ),
    'aliases' => array('robliblup'),
  );
  return $items;
}

function roblib_get_all_perms() {
  $all_perms = array();
  foreach (module_list(FALSE, FALSE, TRUE) as $module) {
    if ($permissions = module_invoke($module, 'permission')) {
      foreach($permissions as $key => $value) {
        $all_perms[] = $key;
      }
    }
  }
  return $all_perms;
}

function drush_roblib_perms_list_unused_perms($rid){
  $role_id = array_search($rid, user_roles());
  $role_array = array($role_id => $rid);
  $role_permissions = user_role_permissions($role_array);
  $role_permissions = array_keys($role_permissions[$role_id]);
  $all_perms = roblib_get_all_perms();
  $diff = array_diff($all_perms, $role_permissions);
  foreach($diff as $dif){
     drush_print($dif);
  }
}