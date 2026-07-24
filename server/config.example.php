<?php

$db = new PDO(
    "pgsql:host=localhost;dbname=photobase",
    "",
    ""
);

$db->setAttribute(
    PDO::ATTR_ERRMODE,
    PDO::ERRMODE_EXCEPTION
);