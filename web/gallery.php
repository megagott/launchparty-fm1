<?php
declare(strict_types=1);
header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store');
header('X-Content-Type-Options: nosniff');
function answer(array $a,int $s=200):never {http_response_code($s);echo json_encode($a,JSON_UNESCAPED_UNICODE|JSON_INVALID_UTF8_SUBSTITUTE);exit;}
function validShow(string $b):bool {
 $len=strlen($b);if($len<140||$len>8192||substr($b,0,4)!=='LPS2')return false;
 $u=unpack('Vduration/Vcount',substr($b,4,8));if(!$u['duration']||$u['count']>intdiv($len-140,3))return false;
 for($i=12;$i<140;$i++)if(ord($b[$i])>127)return false;
 $p=140;$t=0;for($i=0;$i<$u['count'];$i++){$delta=0;$shift=0;do{if($p>=$len||$shift>28)return false;$v=ord($b[$p++]);if($shift===28&&($v&127)>15)return false;$delta+=($v&127)*(2**$shift);$shift+=7;}while($v&128);$t+=$delta;if($t>=$u['duration']||$p+2>$len||ord($b[$p])>127||ord($b[$p+1])>127)return false;$p+=2;}return $p===$len;
}
try {
 $dir=__DIR__.'/gallery-data';$action=$_GET['action']??'list';
 if($action==='list'){$items=[];foreach(glob($dir.'/*.json')?:[] as $f){$s=json_decode(file_get_contents($f),true);if(is_array($s))$items[]=array_intersect_key($s,array_flip(['id','name','description','author','created','bytes']));}usort($items,fn($a,$b)=>strcmp($b['created'],$a['created']));answer(['items'=>$items]);}
 if($action==='get'){$id=$_GET['id']??'';if(!preg_match('/^[a-f0-9]{32}$/D',$id)||!is_file($dir.'/'.$id.'.json'))answer(['error'=>'Show nicht gefunden.'],404);$s=json_decode(file_get_contents($dir.'/'.$id.'.json'),true);unset($s['owner']);answer($s);}
 define('HP_APP',1);require __DIR__.'/joomla-session.php';$session=hp_joomla_session(dirname(__DIR__));$user=$session['identity'];
 $token=\Joomla\CMS\Session\Session::getFormToken();
 if($action==='session'){$owned=[];if($user)foreach(glob($dir.'/*.json')?:[] as $f){$s=json_decode(file_get_contents($f),true);if(($s['owner']??0)===$user['id'])$owned[]=$s['id'];}answer(['user'=>$user?['name'=>$user['name']]:null,'token'=>$token,'owned'=>$owned]);}
 if(!$user)answer(['error'=>'Bitte zuerst bei MandaLars anmelden.'],401);
 if($_SERVER['REQUEST_METHOD']!=='POST')answer(['error'=>'POST erforderlich.'],405);
 if((int)($_SERVER['CONTENT_LENGTH']??0)>20000)answer(['error'=>'Datei zu groß.'],413);
 $input=json_decode(file_get_contents('php://input',false,null,0,20001),true,16,JSON_THROW_ON_ERROR);
 if(!is_array($input)||!is_string($input['token']??null)||!hash_equals($token,$input['token']))answer(['error'=>'Sitzung abgelaufen. Bitte Seite neu laden.'],403);
 if($action==='publish'){
  $name=trim((string)($input['name']??''));$description=trim((string)($input['description']??''));$data=$input['data']??null;
  if(!$name||strlen($name)>100||strlen($description)>1000||!is_string($data)||strlen($data)>11000)answer(['error'=>'Name oder Showdaten ungültig.'],422);
  $raw=base64_decode($data,true);if($raw===false||!validShow($raw))answer(['error'=>'Ungültige LPS2-Show.'],422);
  if(!is_dir($dir)||!is_file($dir.'/.htaccess'))throw new RuntimeException('Galeriespeicher fehlt.');
  $lock=fopen($dir.'/write.lock','c');if(!$lock||!flock($lock,LOCK_EX))throw new RuntimeException('Galerie beschäftigt.');
  $owned=0;foreach(glob($dir.'/*.json')?:[] as $f){$s=json_decode(file_get_contents($f),true);if(($s['owner']??0)===$user['id']){$owned++;if(time()-strtotime($s['created'])<20)answer(['error'=>'Bitte kurz vor der nächsten Veröffentlichung warten.'],429);}}
  if($owned>=200)answer(['error'=>'Maximal 200 eigene Shows.'],422);
  $id=bin2hex(random_bytes(16));$s=['id'=>$id,'name'=>$name,'description'=>$description,'author'=>$user['name'],'owner'=>$user['id'],'created'=>gmdate('c'),'bytes'=>strlen($raw),'format'=>'launchshow','version'=>1,'data'=>base64_encode($raw)];
  $tmp=$dir.'/'.$id.'.tmp';if(file_put_contents($tmp,json_encode($s,JSON_UNESCAPED_UNICODE|JSON_THROW_ON_ERROR),LOCK_EX)===false||!rename($tmp,$dir.'/'.$id.'.json'))throw new RuntimeException('Speichern fehlgeschlagen.');flock($lock,LOCK_UN);fclose($lock);unset($s['owner']);answer($s,201);
 }
 if($action==='delete'){$id=$input['id']??'';if(!is_string($id)||!preg_match('/^[a-f0-9]{32}$/D',$id)||!is_file($dir.'/'.$id.'.json'))answer(['error'=>'Show nicht gefunden.'],404);$s=json_decode(file_get_contents($dir.'/'.$id.'.json'),true);if(($s['owner']??0)!==$user['id'])answer(['error'=>'Nur eigene Shows können gelöscht werden.'],403);if(!unlink($dir.'/'.$id.'.json'))throw new RuntimeException('Löschen fehlgeschlagen.');answer(['ok'=>true]);}
 answer(['error'=>'Unbekannte Aktion.'],400);
}catch(Throwable $e){error_log('Launchparty gallery: '.$e->getMessage());answer(['error'=>'Galerie derzeit nicht erreichbar. Bitte später erneut versuchen.'],500);}
