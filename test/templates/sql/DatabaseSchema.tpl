{#
  Template for db.sql, mysql dump"
  - name
  - host
  - tables
  - idtables
-#}
## file "db.sql" 
-- phpMyAdmin SQL Dump
-- version 3.3.8
-- http://www.phpmyadmin.net
--
-- Host: {{ host }}

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

--
-- Database: `{{ name }}`
--

--
-- Table `session`
--

CREATE TABLE IF NOT EXISTS `session` (
  `id` char(32) NOT NULL DEFAULT '',
  `name` char(32) NOT NULL DEFAULT '',
  `modified` int(11) DEFAULT NULL,
  `lifetime` int(11) DEFAULT NULL,
  `data` text,
  PRIMARY KEY (`id`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

## apply-template TableSchema tables
## apply-template TableSchemaId idtables
## endfile 
