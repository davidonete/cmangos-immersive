-- cleanup
DELETE FROM `gossip_menu` where entry in (60001, 60002, 60002, 60003);
DELETE FROM `gossip_menu_option` where menu_id in (60001, 60002, 60003);
DELETE FROM `gossip_menu_option` where option_text = 'Manage attributes';
DELETE FROM `locales_gossip_menu_option` where menu_id in (60001, 60002, 60003);
DELETE FROM `locales_gossip_menu_option` where option_text_loc6 = 'Administrat atributos';

SET @TEXT_ID := 50800;
DELETE FROM `npc_text` WHERE `ID` in (@TEXT_ID, @TEXT_ID+1, @TEXT_ID+2);
DELETE FROM `locales_npc_text` WHERE `entry` in (@TEXT_ID, @TEXT_ID+1, @TEXT_ID+2);

-- add missing gossips to trainers
update creature_template set GossipMenuId = 0 where entry in (select entry from creature_template_immersive_backup);
update gossip_menu_option
join gossip_menu_option_immersive_backup 
	ON gossip_menu_option.menu_id = gossip_menu_option_immersive_backup.menu_id 
	AND gossip_menu_option.id = gossip_menu_option_immersive_backup.id
set gossip_menu_option.condition_id = gossip_menu_option_immersive_backup.condition_id;

-- chat messages
DELETE FROM `mangos_string` where entry in (12100, 12101, 12102, 12103, 12104, 12105, 12106, 12107, 12108, 12109, 12110, 12111, 12112, 12113, 12114, 12115, 12116, 12117, 12118, 12119, 12120, 12121);

-- backup
drop table if exists creature_template_immersive_backup;
drop table if exists gossip_menu_option_immersive_backup;