/* $Id: admin_menu.js,v 1.7.2.2 2008/06/15 01:45:05 sun Exp $ */

$(document).ready(function() {
  // Apply margin-top if enabled; directly applying marginTop doesn't work in IE.
  if (Drupal.settings.admin_menu.margin_top) {
    $('body').addClass('admin-menu');
  }

  // Collapse fieldsets on Modules page. For why multiple selectors see #111719.
  if (Drupal.settings.admin_menu.tweak_modules) {
    $('#system-modules fieldset:not(.collapsed), #system-modules-1 fieldset:not(.collapsed)').addClass('collapsed');
  }

  // Hover emulation for IE 6.
  if ($.browser.msie && parseInt(jQuery.browser.version) == 6) {
    $('#admin-menu li').hover(function() {
      $(this).addClass('iehover');
    }, function() {
      $(this).removeClass('iehover');
    });
  }

  // Delayed mouseout.
  $('#admin-menu li').hover(function() {
    // Stop the timer.
    clearTimeout(this.sfTimer);
    // Display child lists.
    $('> ul', this).css('left', 'auto')
      // Immediately hide nephew lists.
      .parent().siblings('li').children('ul').css('left', '-999em');
  }, function() {
    // Start the timer.
    var uls = $('> ul', this);
    this.sfTimer = setTimeout(function() {
      uls.css('left', '-999em');
    }, 400);
  });
});
