
<div id="node-<?php print $node->nid; ?>" class="<?php print $classes; ?> clearfix">

    <h2 class="title"><?php print $title; ?></h2>

  <?php if ($unpublished): ?>
    <div class="unpublished"><?php print t('Unpublished'); ?></div>
  <?php endif; ?>

  <div class="content">
    <?php print $content; ?>
  </div>

  
</div> <!-- /.node -->
