<?php

define('RESOURCE_ORIENTATION_VERTICAL', 0);
define('RESOURCE_ORIENTATION_HORIZONTAL', 1);

function resource_get_sections() {
  $sections = array();
  $sql = "SELECT id, machine_name FROM {sections}
    WHERE status = %d
    ORDER BY weight ASC, machine_name ASC";
  $result = db_query($sql, SECTION_STATUS_ENABLED);
  while ($data = db_fetch_object($result)) {
    $sections[] = $data->machine_name;
  }
  
  return $sections;
}

function resource_load($id) {
  $resources = array();
  
  if (!isset($resources[$id])) {
    $section = section_load('resource');
    $resources[$id] = section_entity_load($section, $id);
  }
  
  return $resources[$id];
}

function resource_delete($resource) {
  if (!empty($resource->id)) {
    db_query("DELETE FROM {resource} WHERE id = %d", $resource->id);
  }
  
  return TRUE;
}

function _resource_load_all_conditions() {
  static $conditions;
  
  if (!isset($conditions)) {
    $conditions = array();
    
    $result = db_query("SELECT * FROM {search_conditions}");
    while ($data = db_fetch_object($result)) {
      $conditions[$data->id] = $data;
    }
  }
  
  return $conditions;
}

function resource_load_organizations() {
  static $list;
  
  if (!isset($list)) {
    $list = array();
    $sql = "SELECT t.tid, t.name FROM {term_data} t
      INNER JOIN {vocabulary} v ON v.vid = t.vid
      WHERE v.machine_name = 'organization'
      ORDER BY t.weight ASC, t.name ASC";
    $result = db_query($sql);
    while ($data = db_fetch_object($result)) {
      $list[$data->tid] = $data->name;
    }
  }
  
  return $list;
}

function resource_get_user_organization($uid) {
  $organizations = array();
  
  if (!isset($organizations[$uid])) {
    $sql = "SELECT organization_id FROM {users_organizations} WHERE uid = %d";
    $organizations[$uid] = (int) db_result(db_query($sql, $uid));
  }
  
  return $organizations[$uid];
}

function resource_generate_query($filter, $columns, $bookmark, $subquery = array()) {
  global $user;
  
  $sql_fields = array('r.id AS resource_id');
  $sql_join = array(
    'resource' => array('index' => 'r', 'value' => ''),
  );
  $sql_where = array();
  
  // Restrict resources by oganization
  if (!user_access('resource admin')) {
    $sql_join['employment'] = array(
      'index' => 'emp',
      'value' => "INNER JOIN {resource_employment} emp ON emp.resource_id = r.id",
    );
    $sql_where[] = array(
      'condition' => "emp.organization_id = %d AND emp.employment_dt_to IS NULL",
      'value' => resource_get_user_organization($user->uid),
    );
  }
  
  $fields = _resource_load_all_fields();
  $conditions = _resource_load_all_conditions();
  $i = 1;
  
  if (!empty($bookmark)) {
    $sql_where[] = array(
      'condition' => "BINARY UPPER(SUBSTRING(r.lastname, 1, 1)) = '%s'",
      'value' => $bookmark,
    );
  }
  
  // Apply subquery
  foreach ($subquery as $item) {
    if (empty($sql_join[$item['table_name']])) {
      $index = 't' . $i++;
      $sql_join[$item['table_name']] = array(
        'index' => $index,
        'value' => "LEFT JOIN {" . $item['table_name'] . "} " . $index . " ON " .
          $index . "." . $item['local_key'] . " = r." . $item['query_key'],
      );
    }
    else {
      $index = $sql_join[$item['table_name']]['index'];
    }
    
    $sql_where[] = array(
      'condition' => $index . "." . $item['field_name'] . ' ' . $item['expression'],
    );
  }
  
  foreach ($filter as $item) {
    $field = $fields[$item['field_id']];
    $condition = $conditions[$item['condition_id']];
    
    if (empty($sql_join[$field->table_name])) {
      $index = 't' . $i++;
      $sql_join[$field->table_name] = array(
        'index' => $index,
        'value' => "LEFT JOIN {" . $field->table_name . "} " . $index . " ON " .
          $index . "." . $field->remote_key . " = r.id",
      );
    }
    else {
      $index = $sql_join[$field->table_name]['index'];
    }
    
    if ($condition->num_args == 1) {
      $value = $item['value1'];
    }
    elseif ($condition->num_args == 2) {
      $value = array($item['value1'], $item['value2']);
    }
    
    $sql_where[] = array(
      'condition' => $index . "." . $field->machine_name . ' ' . $condition->expression,
      'value' => $value,
    );
  }
  
  $header = array();
  foreach ($columns as $id) {
    $field = $fields[$id];
    
    if (empty($sql_join[$field->table_name])) {
      $index = 't' . $i++;
      $sql_join[$field->table_name] = array(
        'index' => $index,
        'value' => "LEFT JOIN {" . $field->table_name . "} " . $index . " ON " .
          $index . "." . $field->remote_key . " = r.id",
      );
    }
    else {
      $index = $sql_join[$field->table_name]['index'];
    }
    
    $alias = $index . '_' . $field->machine_name;
    
    if ($field->table_name == 'resource') {
      $sql_fields[] = $index . '.' . $field->machine_name . ' AS ' . $alias;
    }
    else {
      $sql_fields[] = 'GROUP_CONCAT(' . $index . '.' . $field->machine_name . " SEPARATOR '|') AS " . $alias;
    }
    
    $header[$alias] = array(
      'data' => $field->title,
      'field' => $index . '.' . $field->machine_name,
    );
    if ($field->machine_name == 'last_name') {
      $header[$alias]['sort'] = 'asc';
    }
    
    $present_fields[$alias] = array(
      'type' => $field->type,
      'source' => $field->source,
      'required' => $field->required,
    );
  }
  
  $sql = "SELECT " . implode(', ', $sql_fields) . " FROM {resource} r";
  $count_sql = "SELECT COUNT(DISTINCT r.id) AS cnt FROM {resource} r";
  $values = array();
  foreach ($sql_join as $join) {
    if (!empty($join['value'])) {
      $sql .= ' ' . $join['value'];
      $count_sql .= ' ' . $join['value'];
    }
  }
  
  if (!empty($sql_where)) {
    $conditions = array();
    foreach ($sql_where as $item) {
      $conditions[] = $item['condition'];
      if (is_array($item['value'])) {
        foreach ($item['value'] as $value) {
          $values[] = $value;
        }
      }
      else {
        $values[] = $item['value'];
      }
    }
    
    $sql .= ' WHERE ' . implode(' AND ', $conditions);
    $count_sql .= ' WHERE ' . implode(' AND ', $conditions);
  }
  
  $sql .= " GROUP BY r.id " . tablesort_sql($header);
  
  return array(
    'sql' => $sql,
    'count_sql' => $count_sql,
    'values' => $values,
    'header' => $header,
    'fields' => $present_fields,
  );
}

function resource_build_result_table($query, $limit = 0) {
  $header = $query['header'];
  array_unshift($header, '#');
  
  if ($limit) {
    $result = pager_query($query['sql'], $limit, 0, $query['count_sql'], $query['values']);
  }
  else {
    $result = db_query($query['sql'], $query['values']);
  }
  while ($data = db_fetch_array($result)) {
    $row = array(
      l(t('View'), 'resource/' . $data['resource_id']),
    );
    
    foreach ($header as $key => $value) {
      if (!empty($key)) {
        if (empty($data[$key])) {
          $value = '';
        }
        elseif (!empty($query['fields'][$key])) {
          $field = (object) $query['fields'][$key];
          $field->required = TRUE;
          $value = explode('|', $data[$key]);
          foreach ($value as $k => $v) {
            $value[$k] = section_format_value($field, $v);
          }
          
          $value = implode("<br />\n", $value);
        }
        else {
          $value = $data[$key];
        }
        
        $row[] = $value;
      }
    }
    
    $rows[$data['resource_id']] = $row;
  }
  
  return array(
    'header' => $header,
    'rows' => $rows,
  );
}

function resource_export_csv($table, $filename) {
  $delimiter = "\t";
  $tmpfile = tmpfile();
  
  $row = array();
  $table['header'][0] = 'id';
  foreach ($table['header'] as $item) {
    $row[] = (is_array($item) ? $item['data'] : $item);
  }
  fputcsv($tmpfile, $row, $delimiter);
  
  $tr = array(
    '<br />' => ' ',
    "\n" => ' ',
    "\t" => ' ',
  );
  foreach ($table['rows'] as $id => $row) {
    $row[0] = $id;
    foreach ($row as $key => $value) {
      $row[$key] = strip_tags(strtr($value, $tr));
    }
    
    fputcsv($tmpfile, $row, $delimiter);
  }
  
  drupal_set_header("Content-Type: text/csv;");
  drupal_set_header('Content-disposition: attachment; filename="' . $filename . '"');
  
  rewind($tmpfile);
  $buffer = stream_get_contents($tmpfile);
  fclose($tmpfile);
  //$buffer = iconv('utf-8', 'UTF-16LE', $buffer);
  
  print $buffer;
  exit();
}

function _resource_load_all_fields($columns = NULL) {
  static $fields;
  
  if (!isset($fields)) {
    global $language;
    
    $fields = array();
    $sql = "SELECT f.id, f.machine_name, f.type, f.source, f.searchable,
      s.machine_name AS section_machine_name, s.table_name, s.remote_key, fd.title, sd.title AS section
      FROM {section_fields} f
      LEFT JOIN {sections} s ON s.id = f.section_id
      LEFT JOIN {section_field_details} fd ON fd.field_id = f.id AND fd.language = '%s'
      LEFT JOIN {section_details} sd ON sd.section_id = s.id AND sd.language = '%s'
      WHERE s.status = %d AND f.status = %d
      ORDER BY s.weight ASC, sd.title ASC, f.weight ASC, fd.title ASC";
    $values = array(
      $language->language,
      $language->language,
      SECTION_STATUS_ENABLED,
      SECTION_STATUS_ENABLED,
    );
    $result = db_query($sql, $values);
    while ($data = db_fetch_object($result)) {
      $fields[$data->id] = $data;
    }
  }
  
  if (!empty($columns)) {
    $fields += $columns;
  }
  
  return $fields;
}

function resource_get_selected_columns($entity, $default_columns = NULL) {
  static $columns = array();
  static $default = array();
  
  if (!empty($default_columns)) {
    $default = $default_columns;
    $columns = array();
  }
  
  if (!isset($columns[$entity])) {
    if (empty($default)) {
      $def = array(
        'resource' => array(
          'firstname' => 0,
          'lastname' => 0,
          'birthday_dt' => 0,
        ),
      );
      
      $fields = _resource_load_all_fields();
      foreach ($fields as $id => $field) {
        if (isset($def[$field->section_machine_name][$field->machine_name])) {
          $default[] = $id;
        }
      }
      foreach ($def as $data) {
        foreach ($data as $value) {
          if ($value) {
            $default[] = $value;
          }
        }
      }
    }
    
    $columns[$entity] = empty($_SESSION['resources']['selected_columns'][$entity]) ?
      array() : $_SESSION['resources']['selected_columns'][$entity];
    foreach (array_reverse($default) as $id) {
      if (!in_array($id, $columns[$entity])) {
        array_unshift($columns[$entity], $id);
      }
    }
  }
  
  return $columns[$entity];
}

function resource_extension_load($section_id) {
  static $extensions = array();
  
  if (!isset($extensions[$section_id])) {
    $extensions[$section_id] = db_fetch_object(db_query("SELECT * FROM {resource_sections} WHERE section_id = %d", $section_id));
  }
  
  return $extensions[$section_id];
}
