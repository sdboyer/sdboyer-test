<?php
// $Id: handlers.inc,v 1.88 2008/06/25 22:20:01 merlinofchaos Exp $
/**
 * @file handlers.inc
 * Defines the various handler objects to help build and display views.
 */

/**
 * Instantiate and construct a new handler
 */
function _views_create_handler($definition) {
//  vpr('Instantiating handler ' . $definition['handler']);
  if (empty($definition['handler']) || !class_exists($definition['handler'])) {
    return;
  }

  $handler = new $definition['handler'];
  $handler->set_definition($definition);
  // let the handler have something like a constructor.
  // @todo: deprecated. Remove.
  if (isset($definition['arguments'])) {
    call_user_func_array(array(&$handler, 'construct'), $definition['arguments']);
  }
  else {
    $handler->construct();
  }

  return $handler;
}

/**
 * Prepare a handler's data by checking defaults and such.
 */
function _views_prepare_handler($definition, $data, $field) {
  foreach (array('group', 'title', 'help', 'real field') as $key) {
    // First check the field level
    if (!isset($definition[$key]) && !empty($data[$field][$key])) {
      $definition[$key] = $data[$field][$key];
    }
    // Then if that doesn't work, check the table level
    else if (!isset($definition['table'][$key]) && !empty($data['table'][$key])) {
      $definition[$key] = $data['table'][$key];
    }
  }

  return _views_create_handler($definition);
}

/**
 * Fetch a handler to join one table to a primary table from the data cache
 */
function views_get_table_join($table, $base_table) {
  $data = views_fetch_data($table);
  if (isset($data['table']['join'][$base_table])) {
    $h = $data['table']['join'][$base_table];
    if (!empty($h['handler']) && class_exists($h['handler'])) {
      $handler = new $h['handler'];
    }
    else {
      $handler = new views_join();
    }

    // Fill in some easy defaults
    $handler->definition = $h;
    if (empty($handler->definition['table'])) {
      $handler->definition['table'] = $table;
    }
    // If this is empty, it's a direct link.
    if (empty($handler->definition['left_table'])) {
      $handler->definition['left_table'] = $base_table;
    }

    if (isset($h['arguments'])) {
      call_user_func_array(array(&$handler, 'construct'), $h['arguments']);
    }
    else {
      $handler->construct();
    }

    return $handler;
  }
  // DEBUG -- identify missing handlers
  vpr("Missing join: $table $base_table");
}

/**
 * Base handler, from which all the other handlers are derived.
 * It creates a common interface to create consistency amongst
 * handlers and data.
 *
 * This class would be abstract in PHP5, but PHP4 doesn't understand that.
 *
 * Definition terms:
 * - table: The actual table this uses; only specify if different from
 *          the table this is attached to.
 * - real field: The actual field this uses; only specify if different from
 *               the field this item is attached to.
 * - group: A text string representing the 'group' this item is attached to,
 *          for display in the UI. Examples: "Node", "Taxonomy", "Comment",
 *          "User", etc. This may be inherited from the parent definition or
 *          the 'table' definition.
 * - title: The title for this handler in the UI. This may be inherited from
 *          the parent definition or the 'table' definition.
 * - help: A more informative string to give to the user to explain what this
 *         field/handler is or does.
 * - access callback: If this field should have access control, this could
 *                    be a function to use. 'user_access' is a common
 *                    function to use here. If not specified, no access
 *                    control is provided.
 * - access arguments: An array of arguments for the access callback.
 */
class views_handler extends views_object {
  /**
   * init the handler with necessary data.
   * @param $view
   *   The $view object this handler is attached to.
   * @param $options
   *   The item from the database; the actual contents of this will vary
   *   based upon the type of handler.
   */
  function init(&$view, $options) {
    $this->view = &$view;
    $this->unpack_options($this->options, $options);

    // This exist on most handlers, but not all. So they are still optional.
    if (isset($options['table'])) {
      $this->table = $options['table'];
    }

    if (isset($this->definition['real field'])) {
      $this->real_field = $this->definition['real field'];
    }

    if (isset($this->definition['field'])) {
      $this->real_field = $this->definition['field'];
    }

    if (isset($options['field'])) {
      $this->field = $options['field'];
      if (!isset($this->real_field)) {
        $this->real_field = $options['field'];
      }
    }

    if (!empty($view->query)) {
      $this->query = &$view->query;
    }
  }

  /**
   * Return a string representing this handler's name in the UI.
   */
  function ui_name() {
    return t('!group: !title', array('!group' => $this->definition['group'], '!title' => $this->definition['title']));
  }

  /**
   * Provide a form for setting options.
   */
  function options_form(&$form, &$form_state) { }

  /**
   * Validate the options form.
   */
  function options_validate($form, &$form_state) { }

  /**
   * Perform any necessary changes to the form values prior to storage.
   * There is no need for this function to actually store the data.
   */
  function options_submit($form, &$form_state) { }

  /**
   * If a handler has 'extra options' it will get a little settings widget and
   * another form called extra_options.
   */
  function has_extra_options() { return FALSE; }

  /**
   * Provide defaults for the handler.
   */
  function extra_options(&$option) { }

  /**
   * Provide a form for setting options.
   */
  function extra_options_form(&$form, &$form_state) { }

  /**
   * Validate the options form.
   */
  function extra_options_validate($form, &$form_state) { }

  /**
   * Perform any necessary changes to the form values prior to storage.
   * There is no need for this function to actually store the data.
   */
  function extra_options_submit($form, &$form_state) { }

  /**
   * Set new exposed option defaults when exposed setting is flipped
   * on.
   */
  function expose_options() { }
  /**
   * Render our chunk of the exposed filter form when selecting
   */
  function exposed_form(&$form, &$form_state) { }

  /**
   * Validate the exposed filter form
   */
  function exposed_validate(&$form, &$form_state) { }

  /**
   * Submit the exposed filter form
   */
  function exposed_submit(&$form, &$form_state) { }

  /**
   * Get information about the exposed form for the form renderer.
   *
   * @return
   *   An array with the following keys:
   *   - operator: The $form key of the operator. Set to NULL if no operator.
   *   - value: The $form key of the value. Set to NULL if no value.
   *   - label: The label to use for this piece.
   */
  function exposed_info() { }

 /**
  * Check whether current user has access to this handler.
  *
  * @return boolean
  */
  function access() {
    if (isset($this->definition['access callback']) && function_exists($this->definition['access callback'])) {
      if (isset($this->definition['access arguments']) && is_array($this->definition['access arguments'])) {
        return call_user_func_array($this->definition['access callback'], $this->definition['access arguments']);
      }
      return $this->definition['access callback']();
    }

    return TRUE;
  }

  /**
   * Run before the view is built.
   *
   * This gives all the handlers some time to set up before any handler has
   * been fully run.
   */
  function pre_query() { }

  /**
   * Called just prior to query(), this lets a handler set up any relationship
   * it needs.
   */
  function set_relationship() {
    // Ensure this gets set to something.
    $this->relationship = NULL;

    // Don't process non-existant relationships.
    if (empty($this->options['relationship']) || $this->options['relationship'] == 'none') {
      return;
    }

    $relationship = $this->options['relationship'];

    // Ignore missing/broken relationships.
    if (empty($this->view->relationship[$relationship])) {
      return;
    }

    // Check to see if the relationship has already processed. If not, then we
    // cannot process it.
    if (empty($this->view->relationship[$relationship]->alias)) {
      return;
    }

    // Finally!
    $this->relationship = $this->view->relationship[$relationship]->alias;
  }

  /**
   * Add this handler into the query.
   *
   * If we were using PHP5, this would be abstract.
   */
  function query() { }

  /**
   * Ensure the main table for this handler is in the query. This is used
   * a lot.
   */
  function ensure_my_table() {
    if (!isset($this->table_alias)) {
      $this->table_alias = $this->query->ensure_table($this->table, $this->relationship);
    }
    return $this->table_alias;
  }

  /**
   * Provide text for the administrative summary
   */
  function admin_summary() { }

  /**
   * Determine if the argument needs a style plugin.
   *
   * @return TRUE/FALSE
   */
  function needs_style_plugin() { return FALSE; }

  /**
   * Determine if this item is 'exposed', meaning it provides form elements
   * to let users modify the view.
   *
   * @return TRUE/FALSE
   */
  function is_exposed() {
    return !empty($this->options['exposed']);
  }

  /**
   * Take input from exposed filters and assign to this handler, if necessary.
   */
  function accept_exposed_input($input) { return TRUE; }

  /**
   * If set to remember exposed input in the session, store it there.
   */
  function store_exposed_input($input, $status) { return TRUE; }

  /**
   * Get the join object that should be used for this handler.
   *
   * This method isn't used a great deal, but it's very handy for easily
   * getting the join if it is necessary to make some changes to it, such
   * as adding an 'extra'.
   */
  function get_join() {
    // get the join from this table that links back to the base table.
    return drupal_clone(views_get_table_join($this->table, $this->query->base_table));
  }

  /**
   * Determine if the handler is considered 'broken', meaning it's a
   * a placeholder used when a handler can't be found.
   */
  function broken() { }
}

/**
 * This many to one helper object is used on both arguments and filters.
 *
 * @todo This requires extensive documentation on how this class is to
 * be used. For now, look at the arguments and filters that use it. Lots
 * of stuff is just pass-through but there are definitely some interesting
 * areas where they interact.
 *
 * Any handler that uses this can have the following possibly additional
 * definition terms:
 * - numeric: If true, treat this field as numeric, using %d instead of %s in
 *            queries.
 *
 */
class views_many_to_one_helper {
  function views_many_to_one_helper(&$handler) {
    $this->handler = &$handler;
  }

  function option_definition(&$options) {
    $options['reduce_duplicates'] = array('default' => FALSE);
  }

  function options_form(&$form, &$form_state) {
    $form['reduce_duplicates'] = array(
      '#type' => 'checkbox',
      '#title' => t('Reduce duplicates'),
      '#description' => t('This filter can cause items that have more than one of the selected options to appear as duplicate results. If this filter causes duplicate results to occur, this checkbox can reduce those duplicates; however, the more terms it has to search for, the less performant the query will be, so use this with caution.'),
      '#default_value' => !empty($this->handler->options['reduce_duplicates']),
    );
  }

  /**
   * Sometimes the handler might want us to use some kind of formula, so give
   * it that option. If it wants us to do this, it must set $helper->formula = TRUE
   * and implement handler->get_formula();
   */
  function get_field() {
    if (!empty($this->formula)) {
      return $this->handler->get_formula();
    }
    else {
      return $this->handler->table_alias . '.' . $this->handler->real_field;
    }
  }

  /**
   * Add a table to the query.
   *
   * This is an advanced concept; not only does it add a new instance of the table,
   * but it follows the relationship path all the way down to the relationship
   * link point and adds *that* as a new relationship and then adds the table to
   * the relationship, if necessary.
   */
  function add_table($join = NULL, $alias = NULL) {
    // This is used for lookups in the many_to_one table.
    $field = $this->handler->table . '.' . $this->handler->field;

    if (empty($join)) {
      $join = $this->get_join();
    }

    // See if there's a chain between us and the base relationship. If so, we need
    // to create a new relationship to use.
    $relationship = $this->handler->relationship;

    // Determine the primary table to seek
    if (empty($this->handler->query->relationships[$relationship])) {
      $base_table = $this->handler->query->base_table;
    }
    else {
      $base_table = $this->handler->query->relationships[$relationship]['base'];
    }

    // Cycle through the joins. This isn't as error-safe as the normal
    // ensure_path logic. Perhaps it should be.
    $r_join = drupal_clone($join);
    while ($r_join->left_table != $base_table) {
      $r_join = views_get_table_join($r_join->left_table, $base_table);
    }

    // If we found that there are tables in between, add the relationship.
    if ($r_join->table != $join->table) {
      $relationship = $this->handler->query->add_relationship(NULL, $r_join, $r_join->table, $this->handler->relationship);
    }

    // And now add our table, using the new relationship if one was used.
    $alias = $this->handler->query->add_table($this->handler->table, $relationship, $join, $alias);

    // Store what values are used by this table chain so that other chains can
    // automatically discard those values.
    if (empty($this->handler->view->many_to_one_tables[$field])) {
      $this->handler->view->many_to_one_tables[$field] = $this->handler->value;
    }
    else {
      $this->handler->view->many_to_one_tables[$field] = array_merge($this->handler->view->many_to_one_tables[$field], $this->handler->value);
    }

    return $alias;
  }

  function get_join() {
    return $this->handler->get_join();
  }

  /**
   * Provide the proper join for summary queries. This is important in part because
   * it will cooperate with other arguments if possible.
   */
  function summary_join() {
    $field = $this->handler->table . '.' . $this->handler->field;
    $join = $this->get_join();

    // shortcuts
    $options = $this->handler->options;
    $view = &$this->handler->view;
    $query = &$this->handler->query;

    if (!empty($options['require_value'])) {
      $join->type = 'INNER';
    }

    if (empty($options['add_table']) || empty($view->many_to_one_tables[$field])) {
      return $query->ensure_table($this->handler->table, $this->handler->relationship, $join);
    }
    else {
      if (!empty($view->many_to_one_tables[$field])) {
        foreach ($view->many_to_one_tables[$field] as $value) {
          $join->extra = array(
            array(
              'field' => $this->handler->real_field,
              'operator' => '!=',
              'value' => $value,
              'numeric' => !empty($this->definition['numeric']),
            ),
          );
        }
      }
      return $this->add_table($join);
    }
  }

  /**
   * Override ensure_my_table so we can control how this joins in.
   * The operator actually has influence over joining.
   */
  function ensure_my_table() {
    if (!isset($this->handler->table_alias)) {
      // For 'or' if we're not reducing duplicates, we get the absolute simplest:
      $field = $this->handler->table . '.' . $this->handler->field;
      if ($this->handler->operator == 'or' && empty($this->handler->options['reduce_duplicates'])) {
        if (empty($this->handler->options['add_table']) && empty($this->handler->view->many_to_one_tables[$field])) {
          // query optimization, INNER joins are slightly faster, so use them
          // when we know we can.
          $join = $this->get_join();
          $join->type = 'INNER';
          $this->handler->table_alias = $this->handler->query->ensure_table($this->handler->table, $this->handler->relationship, $join);
          $this->handler->view->many_to_one_tables[$field] = $this->handler->value;
        }
        else {
          $join = $this->get_join();
          $join->type = 'LEFT';
          if (!empty($this->handler->view->many_to_one_tables[$field])) {
            foreach ($this->handler->view->many_to_one_tables[$field] as $value) {
              $join->extra = array(
                array(
                  'field' => $this->handler->real_field,
                  'operator' => '!=',
                  'value' => $value,
                  'numeric' => !empty($this->handler->definition['numeric']),
                ),
              );
            }
          }

          $this->handler->table_alias = $this->add_table($join);
        }

        return $this->handler->table_alias;
      }

      if ($this->handler->operator != 'not') {
        // If it's an and or an or, we do one join per selected value.
        // Clone the join for each table:
        $this->handler->table_aliases = array();
        foreach ($this->handler->value as $value) {
          $join = $this->get_join();
          if ($this->handler->operator == 'and') {
            $join->type = 'INNER';
          }
          $join->extra = array(
            array(
              'field' => $this->handler->real_field,
              'value' => $value,
              'numeric' => !empty($this->handler->definition['numeric']),
            ),
          );
          $alias = $this->handler->table_aliases[$value] = $this->add_table($join, $this->handler->table . '_' . $value);

          // and set table_alias to the first of these.
          if (empty($this->handler->table_alias)) {
            $this->handler->table_alias = $alias;
          }
        }
      }
      else {
        // For not, we just do one join. We'll add a where clause during
        // the query phase to ensure that $table.$field IS NULL.
        $join = $this->get_join();
        $join->type = 'LEFT';
        $join->extra = array();
        $join->extra_type = 'OR';
        foreach ($this->handler->value as $value) {
          $join->extra[] = array(
            'field' => $this->handler->real_field,
            'value' => $value,
            'numeric' => !empty($this->handler->definition['numeric']),
          );
        }

        $this->handler->table_alias = $this->add_table($join);
      }
    }
    return $this->handler->table_alias;
  }

  function add_filter() {
    if (empty($this->handler->value)) {
      return;
    }
    $this->handler->ensure_my_table();

    // Shorten some variables:
    $field = $this->get_field();
    $options = $this->handler->options;
    $operator = $this->handler->operator;
    if (empty($options['group'])) {
      $options['group'] = 0;
    }

    $placeholder = !empty($this->handler->definition['numeric']) ? '%d' : "'%s'";

    if ($operator == 'not') {
      $this->handler->query->add_where($options['group'], "$field IS NULL");
    }
    else if ($operator == 'or' && empty($options['reduce_duplicates'])) {
      if (count($this->handler->value) > 1) {
        $replace = array_fill(0, sizeof($this->handler->value), $placeholder);
        $in = '(' . implode(", ", $replace) . ')';
        $this->handler->query->add_where($options['group'], "$field IN $in", $this->handler->value);
      }
      else {
        $this->handler->query->add_where($options['group'], "$field = $placeholder", $this->handler->value);
      }
    }
    else {
      $field = $this->handler->real_field;
      $clauses = array();
      foreach ($this->handler->table_aliases as $value => $alias) {
        $clauses[] = "$alias.$field = $placeholder";
      }

      $group = empty($options['group']) ? 0 : $options['group'];

      // implode on either AND or OR.
      $this->handler->query->add_where($group, implode(' ' . strtoupper($operator) . ' ', $clauses), $this->handler->value);
    }
  }
}

/*
 * Break x,y,z and x+y+z into an array. Numeric only.
 *
 * @param $str
 *   The string to parse.
 * @param $filter
 *   The filter object to use as a base. If not specified one will
 *   be created.
 *
 * @return $filter
 *   The new filter object.
 */
function views_break_phrase($str, $filter = NULL) {
  if (!$filter) {
    $filter = new stdClass();
  }
  if (preg_match('/^([0-9]+[+ ])+[0-9]+$/', $str)) {
    // The '+' character in a query string may be parsed as ' '.
    $filter->operator = 'or';
    $filter->value = preg_split('/[+ ]/', $str);
  }
  else if (preg_match('/^([0-9]+,)*[0-9]+$/', $str)) {
    $filter->operator = 'and';
    $filter->value = explode(',', $str);
  }

  // Keep an 'error' value if invalid strings were given.
  if (!empty($str) && (empty($filter->value) || !is_array($filter->value))) {
    $filter->value = array(-1);
    return $filter;
  }

  // Doubly ensure that all values are numeric only.
  foreach ($filter->value as $id => $value) {
    $filter->value[$id] = intval($value);
  }

  return $filter;
}

// --------------------------------------------------------------------------
// Date helper functions

/**
 * Figure out what timezone we're in; needed for some date manipulations.
 */
function views_get_timezone() {
  global $user;
  if (variable_get('configurable_timezones', 1) && $user->uid && strlen($user->timezone)) {
    $timezone = $user->timezone;
  }
  else {
    $timezone = variable_get('date_default_timezone', 0);
  }

  // set up the database timezone
  if (in_array($GLOBALS['db_type'], array('mysql', 'mysqli'))) {
    static $already_set = false;
    if (!$already_set) {
      if ($GLOBALS['db_type'] == 'mysqli' || version_compare(mysql_get_server_info(), '4.1.3', '>=')) {
        db_query("SET @@session.time_zone = '+00:00'");
      }
      $already_set = true;
    }
  }

  return $timezone;
}

/**
 * Helper function to create cross-database SQL dates.
 *
 * @param $field
 *   The real table and field name, like 'tablename.fieldname'.
 * @param $field_type
 *   The type of date field, 'int' or 'datetime'.
 * @param $set_offset
 *   The name of a field that holds the timezone offset or a fixed timezone
 *   offset value. If not provided, the normal Drupal timezone handling
 *   will be used, i.e. $set_offset = 0 will make no timezone adjustment.
 * @return
 *   An appropriate SQL string for the db type and field type.
 */
function views_date_sql_field($field, $field_type = 'int', $set_offset = NULL) {
  $db_type = $GLOBALS['db_type'];
  $offset = $set_offset !== NULL ? $set_offset : views_get_timezone();
  switch ($db_type) {
    case 'mysql':
    case 'mysqli':
      switch ($field_type) {
        case 'int':
          $field = "FROM_UNIXTIME($field)";
          break;
        case 'datetime':
          break;
      }
      if (!empty($offset)) {
        $field = "($field + INTERVAL $offset SECOND)";
      }
      return $field;
    case 'pgsql':
      switch ($field_type) {
        case 'int':
          $field = "$field::ABSTIME";
          break;
        case 'datetime':
          break;
      }
      if (!empty($offset)) {
        $field = "($field + INTERVAL '$offset SECONDS')";
      }
      return $field;
  }
}

/**
 * Helper function to create cross-database SQL date formatting.
 *
 * @param $format
 *   A format string for the result, like 'Y-m-d H:i:s'.
 * @param $field
 *   The real table and field name, like 'tablename.fieldname'.
 * @param $field_type
 *   The type of date field, 'int' or 'datetime'.
 * @param $set_offset
 *   The name of a field that holds the timezone offset or a fixed timezone
 *   offset value. If not provided, the normal Drupal timezone handling
 *   will be used, i.e. $set_offset = 0 will make no timezone adjustment.
 * @return
 *   An appropriate SQL string for the db type and field type.
 */
function views_date_sql_format($format, $field, $field_type = 'int', $set_offset = NULL) {
  $db_type = $GLOBALS['db_type'];
  $field = views_date_sql_field($field, $field_type, $set_offset);
  switch ($db_type) {
    case 'mysql':
    case 'mysqli':
      $replace = array(
        'Y' => '%Y',
        'm' => '%m',
        'd' => '%%d',
        'H' => '%H',
        'i' => '%i',
        's' => '%s',
        );
      $format = strtr($format, $replace);
      return "DATE_FORMAT($field, '$format')";
    case 'pgsql':
      $replace = array(
        'Y' => 'YY',
        'm' => 'MM',
        'd' => 'DD',
        'H' => 'HH24',
        'i' => 'MI',
        's' => 'SS',
        );
      $format = strtr($format, $replace);
      return "TO_CHAR($field, '$format')";
  }
}

/**
 * Helper function to create cross-database SQL date extraction.
 *
 * @param $extract_type
 *   The type of value to extract from the date, like 'MONTH'.
 * @param $field
 *   The real table and field name, like 'tablename.fieldname'.
 * @param $field_type
 *   The type of date field, 'int' or 'datetime'.
 * @param $set_offset
 *   The name of a field that holds the timezone offset or a fixed timezone
 *   offset value. If not provided, the normal Drupal timezone handling
 *   will be used, i.e. $set_offset = 0 will make no timezone adjustment.
 * @return
 *   An appropriate SQL string for the db type and field type.
 */
function views_date_sql_extract($extract_type, $field, $field_type = 'int', $set_offset = NULL) {
  $db_type = $GLOBALS['db_type'];
  $field = views_date_sql_field($field, $field_type, $set_offset);

  // Note there is no space after FROM to avoid db_rewrite problems
  // see http://drupal.org/node/79904.
  switch ($extract_type) {
  case('DATE'):
    return $field;
  case('YEAR'):
    return "EXTRACT(YEAR FROM($field))";
  case('MONTH'):
    return "EXTRACT(MONTH FROM($field))";
  case('DAY'):
    return "EXTRACT(DAY FROM($field))";
  case('HOUR'):
    return "EXTRACT(HOUR FROM($field))";
  case('MINUTE'):
    return "EXTRACT(MINUTE FROM($field))";
  case('SECOND'):
    return "EXTRACT(SECOND FROM($field))";
  case('WEEK'):  // ISO week number for date
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        // WEEK using arg 3 in mysql should return the same value as postgres EXTRACT
        return "WEEK($field, 3)";
      case('pgsql'):
        return "EXTRACT(WEEK FROM($field))";
    }
  case('DOW'):
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        // mysql returns 1 for Sunday through 7 for Saturday
        // php date functions and postgres use 0 for Sunday and 6 for Saturday
        return "INTEGER(DAYOFWEEK($field) - 1)";
      case('pgsql'):
        return "EXTRACT(DOW FROM($field))";
    }
  case('DOY'):
    switch ($db_type) {
      case('mysql'):
      case('mysqli'):
        return "DAYOFYEAR($field)";
      case('pgsql'):
        return "EXTRACT(DOY FROM($field))";
    }
  }

}
