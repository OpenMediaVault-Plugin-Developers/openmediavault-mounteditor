<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @copyright Copyright (c) 2024-2025 openmediavault plugin developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
class OMVRpcServiceMountEditor extends \OMV\Rpc\ServiceAbstract
{
    public function getName()
    {
        return 'MountEditor';
    }

    public function initialize()
    {
        $this->registerMethod('list');
        $this->registerMethod('get');
        $this->registerMethod('set');
        $this->registerMethod('delete');

        $this->registerMethod('addQuota');
        $this->registerMethod('removeQuota');

        $this->registerMethod('show');
    }

    public function list($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.getlist');
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $objects = $db->getByFilter('conf.system.filesystem.mountpoint', [
            'operator' => 'and',
            'arg0' => [
                'operator' => 'stringNotEquals',
                'arg0' => 'hidden',
                'arg1' => '1'
            ],
            'arg1' => [
                'operator' => 'not',
                'arg0' => [
                    'operator' => 'or',
                    'arg0' => [
                        'operator' => 'stringContains',
                        'arg0' => 'opts',
                        'arg1' => 'bind'
                    ],
                    'arg1' => [
                        'operator' => 'stringContains',
                        'arg0' => 'opts',
                        'arg1' => 'loop'
                    ]
                ]
            ]
        ]);
        // Add additional mount informations
        $objectsAssoc = [];
        foreach ($objects as $objectk => &$objectv) {
            // Add the new properties
            $objectv->add('references', 'integer', 0);
            $objectv->add('mounted', 'boolean', false);
            // get count of uuid in database to determine if referenced
            $cmdArgs = [];
            $cmdArgs[] = 'grep';
            $cmdArgs[] = '-c';
            $cmdArgs[] = sprintf('"%s"', $objectv->get('uuid'));
            $cmdArgs[] = '/etc/openmediavault/config.xml';
            $cmd = new \OMV\System\Process($cmdArgs);
            $cmd->setQuiet(true);
            $output = [];
            $cmd->execute($output, $exitStatus);
            $refs = intval($output[0]);
            if ($refs > 0) $objectv->set('references', ($refs - 1));
            // check /proc/mounts to see if mounted
            $cmdArgs = [];
            $cmdArgs[] = 'grep';
            $cmdArgs[] = '-qw';
            $cmdArgs[] = sprintf('"%s"', $objectv->get('dir'));
            $cmdArgs[] = '/proc/mounts';
            $cmd = new \OMV\System\Process($cmdArgs);
            $cmd->setQuiet(true);
            $output = [];
            $cmd->execute($output, $exitStatus);
            if ($exitStatus == 0) $objectv->set('mounted', true);
            // add to list
            $objectsAssoc[] = $objectv->getAssoc();
        }
        // Filter the result.
        return $this->applyFilter($objectsAssoc, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function get($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.objectuuid');
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        return $db->getAssoc('conf.system.filesystem.mountpoint', $params['uuid']);
    }

    public function set($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.system.filesystem.mountpoint', $params['uuid']);
        $opts = explode(',', $params['opts']);
        $optst = array_map('trim', $opts);
        $optsu = array_unique($optst);
        $optsf = implode(',', array_filter($optsu));
        if (strlen($optsf) < 1) {
            throw new \OMV\Exception( gettext("The mount must have at least one mount option!"));
        }
        $object->set('opts', $optsf);
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function delete($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.objectuuid');
        // Delete the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.system.filesystem.mountpoint', $params['uuid']);
        $db->delete($object);
        // Return the deleted configuration object.
        return $object->getAssoc();
    }

    public function addQuota($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.system.filesystem.mountpoint', $params['uuid']);
        $opts = explode(',', $object->get('opts'));
        $optst = array_map('trim', $opts);
        switch ($object->get('type')) {
            case 'ext3':
            case 'ext4':
            case 'f2fs':
                $optadd = ['usrjquota=aquota.user', 'grpjquota=aquota.group', 'jqfmt=vfsv0'];
                break;
            case 'xfs':
            case 'jfs':
                $optadd = ['usrquota', 'grpquota'];
                break;
        }
        $optsm = array_merge($optst, $optadd);
        $optsu = array_unique($optsm);
        $optsf = implode(',', array_filter($optsu));
        if (strlen($optsf) < 1) {
            throw new \OMV\Exception( gettext("The mount must have at least one mount option!"));
        }
        $object->set('opts', $optsf);
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function removeQuota($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.system.filesystem.mountpoint', $params['uuid']);
        $mntpt = $object->get('dir');
        $opts = explode(',', $object->get('opts'));
        $optst = array_map('trim', $opts);
        $optsu = array_unique($optst);
        $optremove = ['usrjquota=aquota.user', 'grpjquota=aquota.group',
            'jqfmt=vfsv0', 'usrquota', 'grpquota'];
        $optsd = array_diff($optsu, $optremove);
        $optsf = implode(',', array_filter($optsd));
        if (strlen($optsf) < 1) {
            throw new \OMV\Exception( gettext("The mount must have at least one mount option!"));
        }
        $object->set('opts', $optsf);
        $db->set($object);
        // Apply changes immmediately
        \OMV\Rpc\Rpc::call('Config', 'applyChanges', [
            'modules' => [ 'fstab', 'monit' ],
            'force' => TRUE
        ], $context);
        // try to remount the mountpoint
        $cmdArgs = [];
        $cmdArgs[] = 'mount';
        $cmdArgs[] = '-o remount';
        $cmdArgs[] = sprintf('"%s"', $mntpt);
        $cmd = new \OMV\System\Process($cmdArgs);
        $cmd->setQuiet(true);
        $output = [];
        $cmd->execute($output, $exitStatus);
        // remove aquota files
        $cmdArgs = [];
        $cmdArgs[] = 'rm';
        $cmdArgs[] = '-f';
        $cmdArgs[] = sprintf('"%s/aquota.user"', $mntpt);
        $cmdArgs[] = sprintf('"%s/aquota.group"', $mntpt);
        $cmd = new \OMV\System\Process($cmdArgs);
        $cmd->setQuiet(true);
        $output = [];
        $cmd->execute($output, $exitStatus);
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function show($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.system.filesystem.mountpoint', $params['uuid']);
        return $this->execBgProc(function($bgStatusFilename, $bgOutputFilename)
            use ($object) {
                $msg = gettext("NOTE: Live options will typically not match because some options are expanded when mounted.");
                $msg .= PHP_EOL . PHP_EOL;
                file_put_contents($bgOutputFilename, $msg, FILE_APPEND);
                $msg = gettext("Path:");
                $msg .= PHP_EOL . '  ' . $object->get('dir') . PHP_EOL . PHP_EOL;
                file_put_contents($bgOutputFilename, $msg, FILE_APPEND);
                $msg = gettext("Current database options:");
                $msg .= PHP_EOL . '  ' . $object->get('opts') . PHP_EOL . PHP_EOL;
                file_put_contents($bgOutputFilename, $msg, FILE_APPEND);
                $msg = gettext("Live options:");
                $msg .= PHP_EOL . '  ';
                file_put_contents($bgOutputFilename, $msg, FILE_APPEND);
                $cmdArgs = [];
                $cmdArgs[] = 'awk';
                $cmdArgs[] = sprintf('\'$2=="%s" { print $4 }\'', $object->get('dir'));
                $cmdArgs[] = '/proc/mounts';
                $cmd = new \OMV\System\Process($cmdArgs);
                $cmd->setRedirect2to1();
                $cmdLine = $cmd->getCommandLine();
                if (0 !== $this->exec($cmdLine, $output, $bgOutputFilename)) {
                    throw new \OMV\ExecException($cmdLine, $output);
                }
                return $output;
            }
        );
    }
}
