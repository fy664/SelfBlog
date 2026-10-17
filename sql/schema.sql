-- =============================================================
-- 全栈个人博客系统 - 数据库初始化脚本
-- 目标数据库：MySQL 8.x（字符集 utf8mb4，支持完整中文与 emoji）
-- 使用方式：mysql -u root -p < schema.sql
-- =============================================================

CREATE DATABASE IF NOT EXISTS blog
  DEFAULT CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

USE blog;

-- -------------------------------------------------------------
-- 1. 用户表 user
--    角色：admin（管理员）/ author（作者）
--    password 存 "盐$SHA256hex" 格式，见后端 util（MVP 说明）
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `user` (
  `id`          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '用户ID',
  `username`    VARCHAR(32)     NOT NULL COMMENT '用户名（登录用，唯一）',
  `password`    VARCHAR(128)    NOT NULL COMMENT '密码散列：salt$sha256hex',
  `avatar`      VARCHAR(255)    DEFAULT NULL COMMENT '头像地址（预留）',
  `role`        ENUM('admin','author') NOT NULL DEFAULT 'author' COMMENT '角色：管理员/作者',
  `register_ip` VARCHAR(64)     NOT NULL DEFAULT '' COMMENT '注册时的客户端 IP（IP 限注册）',
  `created_at`  DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '注册时间',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_username` (`username`),
  KEY `idx_register_ip` (`register_ip`)
) ENGINE=InnoDB COMMENT='用户表';

-- 幂等迁移：对已存在的旧 user 表补充 register_ip 列（CREATE TABLE IF NOT EXISTS 不会改旧表）
SET @col_exists := (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'user' AND COLUMN_NAME = 'register_ip'
);
SET @ddl := IF(@col_exists = 0,
  'ALTER TABLE `user` ADD COLUMN `register_ip` VARCHAR(64) NOT NULL DEFAULT '''' COMMENT ''注册时的客户端 IP（IP 限注册）'', ADD KEY `idx_register_ip` (`register_ip`)',
  'SELECT 1');
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------
-- 2. 文章表 article
--    status：draft（草稿）/ published（已发布）
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `article` (
  `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '文章ID',
  `title`      VARCHAR(200)    NOT NULL COMMENT '标题',
  `content`    LONGTEXT        NOT NULL COMMENT '正文（Markdown 原文）',
  `summary`    VARCHAR(500)    DEFAULT NULL COMMENT '摘要',
  `cover_img`  VARCHAR(255)    DEFAULT NULL COMMENT '封面图（预留）',
  `status`     ENUM('draft','published') NOT NULL DEFAULT 'draft' COMMENT '状态：草稿/发布',
  `view_count` INT UNSIGNED    NOT NULL DEFAULT 0 COMMENT '浏览量',
  `user_id`    BIGINT UNSIGNED NOT NULL COMMENT '作者ID',
  `created_at` DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  `updated_at` DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`id`),
  KEY `idx_status_created` (`status`, `created_at`),
  KEY `idx_user_id` (`user_id`),
  CONSTRAINT `fk_article_user` FOREIGN KEY (`user_id`) REFERENCES `user` (`id`)
) ENGINE=InnoDB COMMENT='文章表';

-- -------------------------------------------------------------
-- 3. 标签表 tag
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `tag` (
  `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '标签ID',
  `name`       VARCHAR(32)     NOT NULL COMMENT '标签名（唯一）',
  `created_at` DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_name` (`name`)
) ENGINE=InnoDB COMMENT='标签表';

-- -------------------------------------------------------------
-- 4. 文章-标签中间表 article_tag
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `article_tag` (
  `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '记录ID',
  `article_id` BIGINT UNSIGNED NOT NULL COMMENT '文章ID',
  `tag_id`     BIGINT UNSIGNED NOT NULL COMMENT '标签ID',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_article_tag` (`article_id`, `tag_id`),
  KEY `idx_tag_id` (`tag_id`),
  CONSTRAINT `fk_at_article` FOREIGN KEY (`article_id`) REFERENCES `article` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_at_tag`     FOREIGN KEY (`tag_id`)     REFERENCES `tag` (`id`)     ON DELETE CASCADE
) ENGINE=InnoDB COMMENT='文章-标签中间表';

-- -------------------------------------------------------------
-- 5. 评论表 comment
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `comment` (
  `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '评论ID',
  `article_id` BIGINT UNSIGNED NOT NULL COMMENT '所属文章ID',
  `user_id`    BIGINT UNSIGNED NOT NULL COMMENT '评论者ID',
  `content`    VARCHAR(2000)   NOT NULL COMMENT '评论内容',
  `created_at` DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '评论时间',
  PRIMARY KEY (`id`),
  KEY `idx_article_id` (`article_id`),
  CONSTRAINT `fk_comment_article` FOREIGN KEY (`article_id`) REFERENCES `article` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_comment_user`   FOREIGN KEY (`user_id`)    REFERENCES `user` (`id`)
) ENGINE=InnoDB COMMENT='评论表';

-- -------------------------------------------------------------
-- 6. 登录日志表 login_log（登录/注册审计）
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `login_log` (
  `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  `username`   VARCHAR(32)     NOT NULL COMMENT '尝试登录的用户名',
  `ip`         VARCHAR(64)     NOT NULL DEFAULT '' COMMENT '客户端 IP',
  `user_agent` VARCHAR(255)    NOT NULL DEFAULT '' COMMENT '浏览器/设备标识',
  `success`    TINYINT(1)      NOT NULL DEFAULT 0 COMMENT '是否登录成功：1成功 0失败',
  `created_at` DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '时间',
  PRIMARY KEY (`id`),
  KEY `idx_username` (`username`),
  KEY `idx_success` (`success`)
) ENGINE=InnoDB COMMENT='登录日志表';

-- -------------------------------------------------------------
-- 预留表（后续版本）：setting（站点配置）、visit_log（访问日志）
-- -------------------------------------------------------------

-- 初始化示例数据：两个常用标签（管理员账号由后端启动时引导创建）
INSERT INTO `tag` (`name`) VALUES ('C++'), ('全栈')
ON DUPLICATE KEY UPDATE `name` = `name`;
