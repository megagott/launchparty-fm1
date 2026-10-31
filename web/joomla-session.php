<?php
declare(strict_types=1);
if(!defined('HP_APP')){http_response_code(404);exit;}

/** Reuse Joomla's frontend session; never authenticate a supplied user ID. */
function hp_joomla_session(string $root): array {
    $root=realpath($root);
    if(!$root || !is_file($root.'/includes/framework.php'))throw new RuntimeException('Joomla nicht erreichbar.');
    if(!defined('_JEXEC'))define('_JEXEC',1);
    if(!defined('JPATH_BASE'))define('JPATH_BASE',$root);
    require_once $root.'/includes/defines.php';
    require_once $root.'/includes/framework.php';
    $container=\Joomla\CMS\Factory::getContainer();
    foreach([\Joomla\Session\SessionInterface::class,\Joomla\Session\Session::class,\Joomla\CMS\Session\Session::class,'session'] as $alias)$container->alias($alias,'session.web.site');
    $app=$container->get(\Joomla\CMS\Application\SiteApplication::class);
    \Joomla\CMS\Factory::$application=$app;
    $app->getSession()->start();
    $stored=$app->getSession()->get('user');
    $identity=null;
    if($stored && (int)$stored->id>0){
        // Recheck database state on every request, including blocked/disabled accounts.
        $fresh=$container->get(\Joomla\CMS\User\UserFactoryInterface::class)->loadUserById((int)$stored->id);
        if((int)$fresh->id>0 && !$fresh->block && empty($fresh->activation) && $fresh->email!=='' && in_array(2,$fresh->getAuthorisedViewLevels(),true)){
            $app->loadIdentity($fresh);
            $identity=['id'=>(int)$fresh->id,'name'=>(string)$fresh->name,'email'=>(string)$fresh->email];
        }
    }
    return ['app'=>$app,'identity'=>$identity];
}
