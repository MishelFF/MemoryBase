<?php

$host = getenv("DB_HOST") ?: "localhost";
$dbname = getenv("DB_NAME") ?: "photobase";
$user = getenv("DB_USER") ?: "postgres";
$password = getenv("DB_PASSWORD") ?: "qwerty";

$db = new PDO(
    "pgsql:host=$host;dbname=$dbname",
    $user,
    $password
);
$db->setAttribute(
    PDO::ATTR_ERRMODE,
    PDO::ERRMODE_EXCEPTION
);