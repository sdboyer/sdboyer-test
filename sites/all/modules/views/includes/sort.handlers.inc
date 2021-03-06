<?php
// $Id: sort.handlers.inc,v 1.6 2008/06/18 15:25:46 merlinofchaos Exp $
/**
 * @file
 * Views' sort handlers.
 */

/**
 * @defgroup views_sort_handlers Views' sort handlers
 * @{
 * Handlers to tell Views how to sort queries
 */

/**
 * Base sort handler that has no options and performs a simple sort
 */
class views_handler_sort extends views_handler {
  /**
   * Called to add the sort to a query.
   */
  function query() {
    $this->ensure_my_table();
    // Add the field.
    $this->query->add_orderby($this->table_alias, $this->real_field, $this->options['order']);
  }

  function option_definition() {
    $options = parent::option_definition();

    $options['order'] = array('default' => 'ASC');

    return $options;
  }

  /**
   * Display whether or not the sort order is ascending or descending
   */
  function admin_summary() {
    switch ($this->options['order']) {
      case 'ASC':
      case 'asc':
      default:
        $type = t('asc');
        break;
      case 'DESC';
      case 'desc';
        $type = t('desc');
        break;
    }
    return '<span class="views-ascending"><span>' . $type . '</span></span>';
  }

  /**
   * Basic options for all sort criteria
   */
  function options_form(&$form, &$form_state) {
    $form['order'] = array(
      '#type' => 'radios',
      '#title' => t('Sort order'),
      '#options' => array('ASC' => t('Ascending'), 'DESC' => t('Descending')),
      '#default_value' => $this->options['order'],
    );
  }
}

/**
 * A special handler to take the place of missing or broken handlers.
 */
class views_handler_sort_broken extends views_handler_sort {
  function ui_name() {
    return t('Broken/missing handler');
  }

  function ensure_my_table() { /* No table to ensure! */ }
  function query() { /* No query to run */ }
  function options_form(&$form, &$form_state) {
    $form['markup'] = array(
      '#prefix' => '<div class="form-item description">',
      '#value' => t('The handler for this item is broken or missing and cannot be used. If a module provided the handler and was disabled, re-enabling the module may restore it. Otherwise, you should probably delete this item.'),
    );
  }

  /**
   * Determine if the handler is considered 'broken'
   */
  function broken() { return TRUE; }
}

/**
 * Base sort handler that has no options and performs a simple sort
 *
 * Definition items:
 * - formula: The formula to use to sort on, such as with a random sort.
 *            The formula should be an array, with keys for database
 *            types, and 'default' for non-specified. 'default' is
 *            required, all others ('mysql', 'mysqli' and 'pgsql' are
 *            optional). It is recommended you use 'default' for mysql
 *            and create specific overrides for pgsql when the formulae
 *            differ.
 */
class views_handler_sort_formula extends views_handler_sort {
  /**
   * Constructor to take the formula this sorts on.
   */
  function construct() {
    $this->formula = $this->definition['formula'];
    if (is_array($this->formula) && !isset($this->formula['default'])) {
      $this->error = t('views_handler_sort_formula missing default: @formula', array('@formula' => var_export($this->formula, TRUE)));
    }
    parent::construct();
  }
  /**
   * Called to add the sort to a query.
   */
  function query() {
    if (is_array($this->formula)) {
      global $db_type;
      if (isset($this->formula[$db_type])) {
        $formula = $this->formula[$db_type];
      }
      else {
        $formula = $this->formula['default'];
      }
    }
    else {
      $formula = $this->formula;
    }
    $this->ensure_my_table();
    // Add the field.
    $this->query->add_orderby(NULL, $formula, $this->options['order'], $this->table_alias . '_' . $this->field);
  }
}

class views_handler_sort_date extends views_handler_sort {
  function option_definition() {
    $options = parent::option_definition();

    $options['granularity'] = array('default' => 'second');

    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['granularity'] = array(
      '#type' => 'radios',
      '#title' => t('Granularity'),
      '#options' => array(
        'second' => t('Second'),
        'minute' => t('Minute'),
        'hour'   => t('Hour'),
        'day'    => t('Day'),
        'month'  => t('Month'),
        'year'   => t('Year'),
      ),
      '#description' => t('The granularity is the smallest unit to use when determining whether two dates are the same; for example, if the granularity is "Year" then all dates in 1999, regardless of when they fall in 1999, will be considered the same date.'),
      '#default_value' => $this->options['granularity'],
    );
  }

  /**
   * Called to add the sort to a query.
   */
  function query() {
    $this->ensure_my_table();
    switch ($this->options['granularity']) {
      case 'second':
      default:
        $this->query->add_orderby($this->table_alias, $this->real_field, $this->options['order']);
        return;
      case 'minute':
        $formula = views_date_sql_format('YmdHi', "$this->table_alias.$this->real_field");
        break;
      case 'hour':
        $formula = views_date_sql_format('YmdH', "$this->table_alias.$this->real_field");
        break;
      case 'day':
        $formula = views_date_sql_format('Ymd', "$this->table_alias.$this->real_field");
        break;
      case 'month':
        $formula = views_date_sql_format('Ym', "$this->table_alias.$this->real_field");
        break;
      case 'year':
        $formula = views_date_sql_format('Y', "$this->table_alias.$this->real_field");
        break;
    }

    // Add the field.
    $this->query->add_orderby(NULL, $formula, $this->options['order'], $this->table_alias . '_' . $this->field . '_' . $this->options['granularity']);
  }
}

/**
 * Sort in menu hierarchy order.
 *
 * Given a field name of 'p' this produces an ORDER BY on p1, p2, ..., p9.
 * This is only really useful for the {menu_links} table.
 */
class views_handler_sort_menu_hierarchy extends views_handler_sort {
  function query() {
    $this->ensure_my_table();
    $max_depth = isset($this->definition['max depth']) ? $this->definition['max depth'] : MENU_MAX_DEPTH;
    for ($i = 1; $i <= $max_depth; ++$i) {
      $this->query->add_orderby($this->table_alias, $this->field . $i, $this->options['order']);
    }
  }
}

/**
 * @}
 */
