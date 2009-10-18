<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

/*
 *	ReactOS Support Database System - RSDB
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('RSDB') )
	{
		die(" ");
	}



	$query_page = mysql_query("SELECT * 
								FROM `rsdb_item_vendor` 
								WHERE `vendor_id` = ". htmlentities($RSDB_SET_vendor) ."
								AND `vendor_visible` = '1'
								ORDER BY `vendor_name` ASC") ;
	
	$result_page = mysql_fetch_array($query_page);
	
if ($result_page['vendor_id']) {
?>

<style type="text/css">
<!--
/* tab colors */
.tab                { background-color : #ffffff; }
.tab_s              { background-color : #5984C3; }
.tab_u              { background-color : #A0B7C9; }

/* tab link colors */
a.tabLink           { text-decoration : none; }
a.tabLink:link      { text-decoration : none; }
a.tabLink:visited   { text-decoration : none; }
a.tabLink:hover     { text-decoration : underline; }
a.tabLink:active    { text-decoration : underline; }

/* tab link size */
p.tabLink_s         { color: navy; font-size : 10pt; font-weight : bold; padding : 0 8px 1px 2px; margin : 0; }
p.tabLink_u         { color: black; font-size : 10pt; padding : 0 8px 1px 2px; margin : 0; }

/* text styles */
.strike 	       { text-decoration: line-through; }
.bold              { font-weight: bold; }
.newstitle         { font-weight: bold; color: purple; }
.title_group       { font-size: 16px; font-weight: bold; color: #5984C3; text-decoration: none; }
.bluetitle:visited { color: #323fa2; text-decoration: none; }

.Stil1 {font-size: xx-small}
.Stil2 {font-size: x-small}
.Stil3 {color: #FFFFFF}
.Stil4 {font-size: xx-small; color: #FFFFFF; }

-->
</style>

	<table align="center" border="0" cellpadding="0" cellspacing="0" width="100%">
        <tr align="left" valign="top">
          <!-- title -->
          <td valign="bottom" width="100%">
            <table border="0" cellpadding="0" cellspacing="0" width="100%">
                <tr>
                  <td class="title_group" nowrap="nowrap">&nbsp;</td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s"><img src="images/white_pixel.gif" alt="" height="1" width="1"></td>
                </tr>
          </table></td>
          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
                <tr align="left" valign="top">
                  <td width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="tab_s" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_active.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="tabLink_s"><a href="<?php echo $RSDB_intern_link_vendor.$RSDB_SET_vendor; ?>" class="tabLink">Overview</a></p></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
          </table></td>
          <!-- end tab -->

          <!-- fill the remaining space -->
          <td valign="bottom" width="10">
            <table border="0" cellpadding="0" cellspacing="0" width="100%">
                <tr valign="bottom">
                  <td class="tab_s"><img src="images/white_pixel.gif" alt="" height="1" width="10"></td>
                </tr>
          </table></td>
        </tr>
</table>

	<h2><?php echo $result_page['vendor_name']; ?></h2>
	<p><font size="3" face="Arial, Helvetica, sans-serif"><?php echo htmlentities($result_page['vendor_fullname']); ?></font></p>
	<p><font size="2" face="Arial, Helvetica, sans-serif"><b>Website:</b> <a href="<?php echo htmlentities($result_page['vendor_url']); ?>" onmousedown="return clk(this.href,'res','')"><?php echo htmlentities($result_page['vendor_url']); ?></a></font></p>
	<p><font size="2" face="Arial, Helvetica, sans-serif"><b>E-Mail:</b> <a href="mailto:<?php echo htmlentities($result_page['vendor_email']); ?>"><?php echo htmlentities($result_page['vendor_email']); ?></a></font></p>
	<p><font size="2" face="Arial, Helvetica, sans-serif"><b>Information:</b> <?php echo wordwrap(nl2br(htmlentities($result_page['vendor_infotext'], ENT_QUOTES))); ?></font></p>
	<p>&nbsp;</p>
	<table width="100%" border="0" cellpadding="1" cellspacing="1">
      <tr bgcolor="#5984C3">
        <td width="20%" title="Item">
          <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>
            <?php
		
		switch ($RSDB_SET_view) {
			case "comp": // Compatibility
				echo "Application";
				break;
			case "pack": // Packages
				echo "Package";
				break;
			default:
				echo "Name";
				break;
		}

		?>
        </strong></font></div></td>
        <td width="30%" title="Description">
          <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Description</strong></font></div></td>
        <td width="15%" title="Award/Medal (best)"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Award</strong></font></div></td>
        <td width="8%" title="Version">
          <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Ver.</strong></font></div></td>
        <td width="20%" title="Compatibility &Oslash;">
          <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Compatibility &Oslash;</strong></font></div></td>
        <td width="7%" title="Status"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Stat.</strong></font></div></td>
      </tr>
      <?php
	
		$query_page = mysql_query("SELECT * 
									FROM `rsdb_groups` 
									WHERE `grpentr_vendor` = " . htmlentities($RSDB_SET_vendor) . "
									AND `grpentr_visible` = '1'
									". $RSDB_intern_code_db_rsdb_groups ."
									ORDER BY `grpentr_name` ASC ;") ;
	
		$farbe1="#E2E2E2";
		$farbe2="#EEEEEE";
		$zaehler="0";
		
		while($result_page = mysql_fetch_array($query_page)) { // Pages
	?>
      <tr>
        <td valign="top" bgcolor="<?php
									$zaehler++;
									if ($zaehler == "1") {
										echo $farbe1;
										$farbe = $farbe1;
									}
									elseif ($zaehler == "2") {
										$zaehler="0";
										echo $farbe2;
										$farbe = $farbe2;
									}
								 ?>" >
          <div align="left"><font size="2" face="Arial, Helvetica, sans-serif"><b><a href="<?php echo $RSDB_intern_link_vendor2_group.$result_page['grpentr_id']; ?>">
            <?php
			$query_entry_vendor = mysql_query("SELECT * 
												FROM `rsdb_item_vendor` 
												WHERE `vendor_id` = " .  mysql_real_escape_string($result_page['grpentr_vendor']) ." ;") ;
			$result_entry_vendor = mysql_fetch_array($query_entry_vendor);
	/*	
			echo $result_entry_vendor['vendor_name']."&nbsp;";
	*/
		  ?>
            <?php echo $result_page['grpentr_name']; ?></a></b>
                <?php
			echo " &nbsp;<i>";
			$query_entry_appver = mysql_query("SELECT DISTINCT (
												`comp_appversion` 
												), `comp_osversion` , `comp_id` , `comp_name` 
												FROM `rsdb_item_comp` 
												WHERE `comp_visible` = '1'
												AND `comp_groupid` = '". mysql_real_escape_string($result_page['grpentr_id']) ."'
												GROUP BY `comp_appversion` 
												ORDER BY `comp_appversion` ASC 
												LIMIT 0 , 15 ;") ;
			while($result_entry_appver = mysql_fetch_array($query_entry_appver)) {
				if ($result_entry_appver['comp_name'] > $result_page['grpentr_name']) {
					echo "<a href=\"".$RSDB_intern_link_group.$result_page['grpentr_id']."&amp;group2=".$result_entry_appver['comp_appversion']."\">".substr($result_entry_appver['comp_name'], strlen($result_page['grpentr_name'])+1 )."</a>, ";
				}
			}
			echo "</i>";
		?>
        </font></div></td>
        <td valign="top" bgcolor="<?php echo $farbe; ?>"><font size="2">
          <?php
	
	if (strlen(htmlentities($result_page['grpentr_description'], ENT_QUOTES)) <= 30) {
		echo $result_page['grpentr_description'];
	}
	else {
		echo substr(htmlentities($result_page['grpentr_description'], ENT_QUOTES), 0, 30)."...";
	}
	 
	  ?>
        </font></td>
        <?php
			$counter_stars_install_sum = 0;
			$counter_stars_function_sum = 0;
			$counter_stars_user_sum = 0;
			$counter_awards_best = 0;
			
			$counter_items = 0;

			$counter_testentries = 0;
			$counter_forumentries = 0;
			$counter_screenshots = 0;

			$query_group_sum_items = mysql_query("SELECT * 
													FROM `rsdb_item_comp` 
													WHERE `comp_groupid` = " . mysql_real_escape_string($result_page['grpentr_id']) . "
													AND `comp_visible` = '1'
													ORDER BY `comp_groupid` DESC ;") ;
			while($result_group_sum_items = mysql_fetch_array($query_group_sum_items)) { 
				$counter_items++;
				if ($counter_awards_best < $result_group_sum_items['comp_award']) {
					$counter_awards_best = $result_group_sum_items['comp_award'];
				}
				$query_count_stars_sum = mysql_query("SELECT * 
								FROM `rsdb_item_comp_testresults` 
								WHERE `test_visible` = '1'
								AND `test_comp_id` = " . $result_group_sum_items['comp_id'] . "
								ORDER BY `test_comp_id` ASC") ;
								
				while($result_count_stars_sum = mysql_fetch_array($query_count_stars_sum)) {
					$counter_stars_install_sum += $result_count_stars_sum['test_result_install'];
					$counter_stars_function_sum += $result_count_stars_sum['test_result_function'];
					$counter_stars_user_sum++;
				}
				
				$query_count_testentries=mysql_query("SELECT COUNT('test_id')
														FROM `rsdb_item_comp_testresults`
														WHERE `test_visible` = '1' 
														AND `test_comp_id` = '".mysql_real_escape_string($result_group_sum_items['comp_id'])."' ;");	
				$result_count_testentries = mysql_fetch_row($query_count_testentries);
				$counter_testentries += $result_count_testentries[0];
				
				// Forum entries:
				$query_count_forumentries=mysql_query("SELECT COUNT('fmsg_id')
														FROM `rsdb_item_comp_forum`
														WHERE `fmsg_visible` = '1' 
														AND `fmsg_comp_id` = '".mysql_real_escape_string($result_group_sum_items['comp_id'])."' ;");	
				$result_count_forumentries = mysql_fetch_row($query_count_forumentries);
				$counter_forumentries += $result_count_forumentries[0];

				// Screenshots:
				$query_count_screenshots=mysql_query("SELECT COUNT('media_id')
														FROM `rsdb_object_media`
														WHERE `media_visible` = '1' 
														AND `media_groupid` = '".mysql_real_escape_string($result_group_sum_items['comp_media'])."' ;");	
				$result_count_screenshots = mysql_fetch_row($query_count_screenshots);
				$counter_screenshots += $result_count_screenshots[0];
			}
?>
        <td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="1">&nbsp;<img src="media/icons/awards/<?php echo draw_award_icon($counter_awards_best); ?>.gif" alt="<?php echo draw_award_name($counter_awards_best); ?>" width="16" height="16" /> <?php echo draw_award_name($counter_awards_best); ?></font></div></td>
        <td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="center"><font size="2">
            <?php 
			
			echo $counter_items;
			
			?>
        </font></div></td>
        <td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2">
            <?php 
			
			echo draw_stars_small($counter_stars_function_sum, $counter_stars_user_sum, 5, "") . " (".$counter_stars_user_sum.")";
			
			?>
        </font></div></td>
        <td valign="top" bgcolor="<?php echo $farbe; ?>" title="<?php echo "Tests: ".$counter_testentries.", Forum entries: ".$counter_forumentries.", Screenshots: ".$counter_screenshots; ?>"><div align="center">
            <table width="100%" border="0" cellpadding="1" cellspacing="1">
              <tr>
                <td width="33%"><div align="center">
                    <?php if ($counter_testentries > 0) { ?>
                    <img src="media/icons/info/test.gif" alt="Compatibility Test Report entries" width="13" height="13">
                    <?php } else { echo "&nbsp;"; } ?>
                </div></td>
                <td width="33%"><div align="center">
                    <?php if ($counter_forumentries > 0) { ?>
                    <img src="media/icons/info/forum.gif" alt="Forum entries" width="13" height="13">
                    <?php } else { echo "&nbsp;"; } ?>
                </div></td>
                <td width="33%"><div align="center">
                    <?php if ($counter_screenshots > 0) { ?>
                    <img src="media/icons/info/screenshot.gif" alt="Screenshots" width="13" height="13">
                    <?php } else { echo "&nbsp;"; } ?>
                </div></td>
              </tr>
            </table>
        </div></td>
      </tr>
      <?php	
		}	// end while
	?>
    </table>
<?php
	}
	else {
		echo "<p>No related database entry found.</p>";
	}
?>
<p>&nbsp;</p>
<?php
	include("inc/tree/tree_vendor_id_maintainer.php");
?>	
